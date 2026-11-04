#include "audio.h"
#include "web.h"
#include <Arduino.h>
#include <SD.h>

void cleanup() {
  if (wav)  { wav->stop();  delete wav;  wav  = nullptr; }
  if (file) {                delete file; file = nullptr; }
}

void playFile(const char* path) {
  if (wav && wav->isRunning()) return;
  cleanup();

  file = new AudioFileSourceSD(path);
  if (!file->isOpen()) {
    webLog("[play] failed to open %s\n", path);
    delete file; file = nullptr;
    return;
  }
  webLog("[play] %s (%u bytes)\n", path, file->getSize());

  wav = new AudioGeneratorWAV();
  if (!wav->begin(file, out)) {
    webLog("[play] WAV decode failed");
    cleanup();
  }
}

bool playRandom(const char* folder) {
  if (wav && wav->isRunning()) {
    webLog("[playRandom] busy, skipping %s\n", folder);
    return false;
  }
  char dirPath[64];
  snprintf(dirPath, sizeof(dirPath), "/%s", folder);
  File dir = SD.open(dirPath);
  if (!dir || !dir.isDirectory()) {
    webLog("[playRandom] folder not found: %s\n", dirPath);
    return false;
  }

  int count = 0;
  File entry;
  while ((entry = dir.openNextFile())) {
    if (!entry.isDirectory()) {
      String name = entry.name();
      if (name.endsWith(".wav") || name.endsWith(".WAV")) count++;
    }
    entry.close();
  }
  dir.close();
  if (count == 0) {
    webLog("[playRandom] no .wav files in %s\n", dirPath);
    return false;
  }
  webLog("[playRandom] %d files in %s, picking #%d\n", count, dirPath, count);

  int pick = random(0, count);
  dir = SD.open(dirPath);
  int i = 0;
  while ((entry = dir.openNextFile())) {
    if (!entry.isDirectory()) {
      String name = entry.name();
      if (name.endsWith(".wav") || name.endsWith(".WAV")) {
        if (i == pick) {
          char path[128];
          snprintf(path, sizeof(path), "%s/%s", dirPath, name.c_str());
          entry.close();
          dir.close();
          playFile(path);
          return wav && wav->isRunning();
        }
        i++;
      }
    }
    entry.close();
  }
  dir.close();
  return false;
}
