#pragma once

#include <Arduino.h>
#include <SD.h>

extern int waterTimmy;
extern int waterLuke;
extern unsigned long lastIncrementTimmy;
extern unsigned long lastIncrementLuke;
extern bool congratsPlaying;

extern unsigned long waterCooldownMs;

void loadWaterCounts();
void saveWaterCounts();
bool canIncrement(unsigned long &lastIncrement);
bool incrementWater(int &counter, unsigned long &lastIncrement, const char* name);
