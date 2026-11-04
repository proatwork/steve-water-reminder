#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include <SD.h>
#include "audio.h"

extern WebServer server;
extern int triggerCm;
extern unsigned long motionCooldownMs;
extern File uploadFile;
extern String uploadPath;

void handleRoot();
void handleList();
void handleUploadChunk();
void handleUploadDone();
void handleDelete();
void handleDeleteFolder();
void handlePlayPath();
void handleDetection();
void handleScheduled();
void handleStop();
void handleSetCooldown();
void handleSetDistance();
void handleStatus();
void handleWater();
void handleWaterSet();
void handleWaterReset();
void handleFolders();
void handleMkdir();
void handleSetWaterCooldown();
void handleSetBacklight();
void handleSerial();
void handleSerialData();
void handleSetActiveHours();

extern int activeStartHour;
extern int activeEndHour;
bool isActiveHours();
void saveConfig();
void loadConfig();
void webLog(const char* fmt, ...);
