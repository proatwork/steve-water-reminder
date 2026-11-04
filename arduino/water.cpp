#include "water.h"
#include "web.h"

static const char* WATER_FILE = "/water.txt";
static const char* KEY_TIMMY = "timmy:";
static const char* KEY_LUKE = "luke:";

int waterTimmy = 0;
int waterLuke = 0;
unsigned long lastIncrementTimmy = 0;
unsigned long lastIncrementLuke = 0;
bool congratsPlaying = false;
unsigned long waterCooldownMs = 900000UL;

void loadWaterCounts() {
  if (!SD.exists(WATER_FILE)) return;
  File f = SD.open(WATER_FILE, FILE_READ);
  if (!f) return;
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.startsWith(KEY_TIMMY)) {
      waterTimmy = line.substring(strlen(KEY_TIMMY)).toInt();
    } else if (line.startsWith(KEY_LUKE)) {
      waterLuke = line.substring(strlen(KEY_LUKE)).toInt();
    }
  }
  f.close();
  webLog("[water] loaded timmy=%d luke=%d\n", waterTimmy, waterLuke);
}

void saveWaterCounts() {
  if (SD.exists(WATER_FILE)) SD.remove(WATER_FILE);
  File f = SD.open(WATER_FILE, FILE_WRITE);
  if (!f) {
    webLog("[water] failed to save");
    return;
  }
  f.printf("%s%d\n%s%d\n", KEY_TIMMY, waterTimmy, KEY_LUKE, waterLuke);
  f.close();
}

bool canIncrement(unsigned long &lastIncrement) {
  return lastIncrement == 0 || millis() - lastIncrement >= waterCooldownMs;
}

bool incrementWater(int &counter, unsigned long &lastIncrement, const char* name) {
  if (congratsPlaying) {
    webLog("[water] %s blocked: congrats playing\n", name);
    return false;
  }
  if (!canIncrement(lastIncrement)) {
    unsigned long remaining = (waterCooldownMs - (millis() - lastIncrement)) / 1000;
    webLog("[water] %s cooldown: %lu s remaining\n", name, remaining);
    return false;
  }
  counter++;
  lastIncrement = millis();
  webLog("[water] %s = %d\n", name, counter);
  saveWaterCounts();
  return true;
}
