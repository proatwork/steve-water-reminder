#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

const int DISPLAY_CE  = 4;
const int DISPLAY_DC  = 16;
const int DISPLAY_RST = 17;
const int DISPLAY_BL  = 2;

void setBacklight(int percent);

void initDisplay();
void updateDisplay(long sonarCm);
