#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoOTA.h>
#include <SD.h>
#include <SPI.h>
#include <time.h>
#include "audio.h"
#include "display.h"
#include "water.h"
#include "web.h"
#include "secrets.h"

const char* ssid     = WIFI_SSID;
const char* password = WIFI_PASSWORD;
const int TRIG_PIN = 27;
const int ECHO_PIN = 33;

const int BTN_1_PIN = 13;
const int BTN_2_PIN = 14;

int triggerCm = 100;
unsigned long motionCooldownMs = 30000UL;
unsigned long lastMotionTrigger = 0;
bool firstMotion = true;

#define SD_CS 5

int activeStartHour = 7;
int activeEndHour = 21;

bool isActiveHours() {
  struct tm t;
  if (!getLocalTime(&t)) return false;
  return t.tm_hour >= activeStartHour && t.tm_hour < activeEndHour;
}

WebServer server(80);
AudioGeneratorWAV *wav  = nullptr;
AudioFileSourceSD *file = nullptr;
AudioOutputI2S    *out  = nullptr;
File uploadFile;
String uploadPath;

long measureDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long us = pulseIn(ECHO_PIN, HIGH, 30000);
  if (us == 0) return -1;
  return us / 58;
}

void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(0));

  if (!SD.begin(SD_CS, SPI, 4000000)) {
    Serial.println("SD init failed - check wiring and card format (FAT32)");
    while (1) delay(1000);
  }
  Serial.println("SD ok");

  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) { delay(250); Serial.print('.'); }
  Serial.println();
  Serial.print("IP: "); Serial.println(WiFi.localIP());
  configTzTime("EET-2EEST,M3.5.0/3,M10.5.0/4", "pool.ntp.org");
  Serial.println("NTP sync started");
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);
  pinMode(BTN_1_PIN, INPUT);
  pinMode(BTN_2_PIN, INPUT);

  out = new AudioOutputI2S(0, AudioOutputI2S::INTERNAL_DAC);
  out->SetOutputModeMono(true);
  out->SetGain(0.5);

  server.on("/",             handleRoot);
  server.on("/detection",    handleDetection);
  server.on("/scheduled",    handleScheduled);
  server.on("/stop",         handleStop);
  server.on("/set_cooldown", handleSetCooldown);
  server.on("/set_distance", handleSetDistance);
  server.on("/status",       handleStatus);
  server.on("/list",         handleList);
  server.on("/playpath",     handlePlayPath);
  server.on("/delete",       HTTP_POST, handleDelete);
  server.on("/deletefolder", HTTP_POST, handleDeleteFolder);
  server.on("/upload",       HTTP_POST, handleUploadDone, handleUploadChunk);
  server.on("/set_active_hours", handleSetActiveHours);
  server.on("/set_water_cooldown", handleSetWaterCooldown);
  server.on("/set_backlight", handleSetBacklight);
  server.on("/serial",       handleSerial);
  server.on("/serial_data",  handleSerialData);
  server.on("/folders",      handleFolders);
  server.on("/mkdir",        HTTP_POST, handleMkdir);
  server.on("/water",        handleWater);
  server.on("/water_set",    HTTP_POST, handleWaterSet);
  server.on("/water_reset",  HTTP_POST, handleWaterReset);
  server.begin();
  Serial.println("HTTP server up");

  ArduinoOTA.setHostname("steve");
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.begin();
  Serial.println("OTA ready");

  initDisplay();
  loadConfig();
  loadWaterCounts();

  if (isActiveHours()) {
    playFile("/startup.wav");
    while (wav && wav->isRunning()) { wav->loop(); }
    cleanup();
  }
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();
  if (wav && wav->isRunning() && !wav->loop()) {
    cleanup();
    congratsPlaying = false;
  }

  static unsigned long lastCheck = 0;
  static unsigned long lastDisplayUpdate = 0;
  static long lastSonarCm = -1;

  if (millis() - lastCheck > 100) {
    lastCheck = millis();
    if (!wav || !wav->isRunning()) {
      long d = measureDistanceCm();
      if (d > 0) {
        Serial.printf("[sonar] %ld cm\n", d);
        lastSonarCm = d;
      }
      if (d > 0 && d < triggerCm && isActiveHours() && (firstMotion || millis() - lastMotionTrigger > motionCooldownMs)) {
        Serial.printf("Distance trigger: %ld cm\n", d);
        firstMotion = false;
        lastMotionTrigger = millis();
        playRandom("detection");
      }
    }
  }

  static unsigned long lastBtn1Press = 0;
  static unsigned long lastBtn2Press = 0;
  const unsigned long BTN_DEBOUNCE_MS = 300;

  if (digitalRead(BTN_1_PIN) == HIGH && millis() - lastBtn1Press > BTN_DEBOUNCE_MS) {
    lastBtn1Press = millis();
    if (incrementWater(waterTimmy, lastIncrementTimmy, "timmy") && isActiveHours()) {
      congratsPlaying = playRandom("congratulations_timmy");
    }
  }
  if (digitalRead(BTN_2_PIN) == HIGH && millis() - lastBtn2Press > BTN_DEBOUNCE_MS) {
    lastBtn2Press = millis();
    if (incrementWater(waterLuke, lastIncrementLuke, "luke") && isActiveHours()) {
      congratsPlaying = playRandom("congratulations_luke");
    }
  }

  if (millis() - lastDisplayUpdate > 500) {
    lastDisplayUpdate = millis();
    updateDisplay(lastSonarCm);
  }
}
