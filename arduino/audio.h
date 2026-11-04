#pragma once

#include <Arduino.h>
#include "AudioFileSourceSD.h"
#include "AudioGeneratorWAV.h"
#include "AudioOutputI2S.h"

extern AudioGeneratorWAV *wav;
extern AudioFileSourceSD *file;
extern AudioOutputI2S    *out;

void cleanup();
void playFile(const char* path);
bool playRandom(const char* folder);
