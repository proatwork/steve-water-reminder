#include "display.h"
#include "audio.h"
#include "water.h"
#include "web.h"
#include <WiFi.h>

extern int triggerCm;
extern unsigned long motionCooldownMs;

static Adafruit_PCD8544 lcd(DISPLAY_DC, DISPLAY_CE, DISPLAY_RST);

static int prevTimmy = -1;
static int prevLuke = -1;
static bool prevActive = true;
static bool sleepDrawn = false;

static void drawCrown(int cx, int cy) {
  // 11px wide, 7px tall crown centered at cx
  int x = cx - 5;
  int y = cy;
  lcd.drawLine(x, y + 6, x + 10, y + 6, BLACK);
  lcd.drawLine(x, y + 6, x, y + 2, BLACK);
  lcd.drawLine(x, y + 2, x + 2, y + 4, BLACK);
  lcd.drawLine(x + 2, y + 4, x + 5, y, BLACK);
  lcd.drawLine(x + 5, y, x + 8, y + 4, BLACK);
  lcd.drawLine(x + 8, y + 4, x + 10, y + 2, BLACK);
  lcd.drawLine(x + 10, y + 2, x + 10, y + 6, BLACK);
}

void setBacklight(int percent) {
  int duty = constrain(percent, 0, 100) * 255 / 100;
  ledcWrite(0, duty);
}

void initDisplay() {
  ledcSetup(0, 5000, 8);
  ledcAttachPin(DISPLAY_BL, 0);
  ledcWrite(0, 255);
  lcd.begin();
  lcd.setRotation(2);
  lcd.setContrast(50);
  lcd.clearDisplay();
  lcd.setTextSize(1);
  lcd.setTextColor(BLACK);
  lcd.println("STEVE");
  lcd.println("Booting...");
  lcd.println(WiFi.localIP().toString());
  lcd.display();
}

static void drawSleepScreen() {
  lcd.clearDisplay();
  lcd.setTextColor(BLACK);

  // Creeper face centered on 84x48 display
  // 24x24 face starting at (30, 2)
  int ox = 30, oy = 2;

  // Head outline
  lcd.drawRect(ox, oy, 24, 24, BLACK);

  // Left eye (4x4 filled)
  lcd.fillRect(ox + 4, oy + 5, 4, 4, BLACK);
  // Right eye (4x4 filled)
  lcd.fillRect(ox + 16, oy + 5, 4, 4, BLACK);

  // Closed eyelids (horizontal lines across eyes)
  lcd.drawLine(ox + 3, oy + 7, ox + 9, oy + 7, BLACK);
  lcd.drawLine(ox + 15, oy + 7, ox + 21, oy + 7, BLACK);

  // Mouth - creeper frown shape
  lcd.fillRect(ox + 10, oy + 13, 4, 2, BLACK);
  lcd.fillRect(ox + 8, oy + 15, 2, 4, BLACK);
  lcd.fillRect(ox + 14, oy + 15, 2, 4, BLACK);
  lcd.fillRect(ox + 8, oy + 15, 8, 2, BLACK);

  // zZz
  lcd.setTextSize(2);
  lcd.setCursor(58, 0);
  lcd.print("z");
  lcd.setTextSize(1);
  lcd.setCursor(68, 2);
  lcd.print("Z");
  lcd.setCursor(74, 0);
  lcd.print("z");

  lcd.setTextSize(1);
  lcd.setCursor(18, 30);
  lcd.print("sleeping...");

  // Still show counters small at bottom
  lcd.setCursor(0, 40);
  lcd.print("T:");
  lcd.print(waterTimmy);
  lcd.setCursor(48, 40);
  lcd.print("L:");
  lcd.print(waterLuke);

  lcd.display();
}

void updateDisplay(long sonarCm) {
  bool active = isActiveHours();

  if (!active) {
    if (!sleepDrawn) {
      drawSleepScreen();
      sleepDrawn = true;
      prevTimmy = -1;
      prevLuke = -1;
    }
    return;
  }

  if (sleepDrawn) {
    sleepDrawn = false;
    prevTimmy = -1;
    prevLuke = -1;
  }

  if (waterTimmy == prevTimmy && waterLuke == prevLuke) return;
  prevTimmy = waterTimmy;
  prevLuke = waterLuke;

  lcd.clearDisplay();
  lcd.setTextColor(BLACK);

  lcd.drawLine(42, 0, 42, 47, BLACK);

  bool timmyLeads = waterTimmy > waterLuke;
  bool lukeLeads = waterLuke > waterTimmy;
  bool tied = waterTimmy == waterLuke;

  if (timmyLeads || tied) drawCrown(21, 0);
  if (lukeLeads || tied) drawCrown(63, 0);

  String tStr = String(waterTimmy);
  int tSize = (waterTimmy >= 100) ? 2 : 3;
  lcd.setTextSize(tSize);
  int tW = tStr.length() * 6 * tSize;
  int tY = 10 + (26 - 8 * tSize) / 2;
  lcd.setCursor((42 - tW) / 2, tY);
  lcd.print(tStr);

  String lStr = String(waterLuke);
  int lSize = (waterLuke >= 100) ? 2 : 3;
  lcd.setTextSize(lSize);
  int lW = lStr.length() * 6 * lSize;
  int lY = 10 + (26 - 8 * lSize) / 2;
  lcd.setCursor(43 + (41 - lW) / 2, lY);
  lcd.print(lStr);

  lcd.setTextSize(1);
  lcd.setCursor(7, 40);
  lcd.print("TIMMY");
  lcd.setCursor(52, 40);
  lcd.print("LUKE");

  lcd.display();
}
