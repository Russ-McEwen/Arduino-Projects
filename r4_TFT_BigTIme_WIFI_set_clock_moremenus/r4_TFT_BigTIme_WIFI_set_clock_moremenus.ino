/* 
  ┌───────────────────────────────────────────────────────────────────┐
  |        RUSSCLOX v2 — UNO R4 WiFi + DIYables 3.5" ILI9486          |
  |         - Futuristic HUD style                                    |
  |         - Centered time layout                                    |
  |         - Dynamic themes                                          |
  |         - Popup theme menu (now multi-page)                       |
  |         - Settings menu with checkboxes                           |
  |         - Resistive touch (4-wire)                                |
  |         - WiFi NTP + RTC                                          |
  |───────────────────────────────────────────────────────────────────|
  |                    😎 Russ McEwen 5/23/26                         |
  |                   Posted to GitHub 5/31/26                        |
  |          Added ABOUT box & VERSION def- RJM 6/2/26                |
  |───────────────────────────────────────────────────────────────────|
  └───────────────────────────────────────────────────────────────────┘
*/
#define VERSION 3
#include <DIYables_TFT_Touch_Shield.h>
#include <WiFiS3.h>
#include <RTC.h>
#include "Orbitron_VariableFont_wght24pt7b.h"
#include "Orbitron_VariableFont_wght12pt7b.h"

// ---------- TFT ----------
DIYables_TFT_RM68140_Shield tft;

// ---------- WiFi ----------
char ssid[] = "FWA_XC436L";
char pass[] = "silk4-sky-store";

// ---------- Time / RTC ----------
RTCTime currentTime;
unsigned long lastMillis = 0;
unsigned long lastNTPSync = 0;
const unsigned long ntpInterval = 2UL * 60UL * 1000UL;  // 2 minutes
const long TZ_OFFSET = -4L * 3600L;                     // EDT

// ---------- Popup ----------
bool popupVisible = false;
enum PopupMode {
  POPUP_NONE,
  POPUP_THEME,
  POPUP_SETTINGS,
  POPUP_THEME2,
  ABOUT_BOX
};
PopupMode popupMode = POPUP_NONE;

// ---------- Color helper ----------
#define RGB(r, g, b) DIYables_TFT::colorRGB(r, g, b)

// ---------- Themes ----------
struct Theme {
  uint16_t bg;
  uint16_t timeColor;
  uint16_t secColor;
  uint16_t dateColor;
  uint16_t accent;
};

bool setting_24h = false;
bool setting_showSeconds = true;
bool setting_autoTheme = true;
bool setting_showWiFi = true;

// --- Human-readable colors ---
#define COLOR_BLACK 0x0000
#define COLOR_BLUE 0x001F
#define COLOR_RED 0xF800
#define COLOR_GREEN 0x07E0
#define COLOR_CYAN 0x07FF
#define COLOR_MAGENTA 0xF81F
#define COLOR_YELLOW 0xFFE0
#define COLOR_WHITE 0xFFFF
#define COLOR_ORANGE 0xFD20
#define COLOR_PURPLE 0x8010
#define COLOR_PINK 0xFC9F
#define COLOR_GRAY 0x8410

#define LTGREY RGB(180, 180, 180)
#define GREY RGB(127, 127, 127)
#define DARKGREY RGB(70, 70, 70)
#define TURQUOISE RGB(0, 128, 120)
#define PINK RGB(255, 128, 192)
#define OLIVE RGB(128, 128, 0)
#define PURPLE RGB(100, 0, 100)
#define AZURE RGB(0, 128, 255)
#define ORANGE RGB(255, 128, 64)
#define WHITE RGB(255, 255, 255)
#define RED RGB(255, 0, 0)
#define GREEN RGB(0, 255, 0)
#define BLUE RGB(0, 0, 255)
#define YELLOW RGB(255, 255, 0)
#define BROWN RGB(140, 70, 0)
#define DKBLUE RGB(0, 0, 95)
#define DKGREEN RGB(0, 85, 0)
#define DKRED RGB(70, 0, 0)
#define DKBROWN RGB(110, 55, 0)
#define ARDUINO_GREEN RGB(0, 95, 95)
#define CHARTRUSE RGB(156, 255, 72)
#define DEEPPURPLE RGB(70, 0, 60)

// ----------------------------------------------|
// SECTION 4: RANDOM COLOR UTILITY --------------|
// ----------------------------------------------|
uint16_t randColor() {
  return RGB(
    random(0, 256),
    random(0, 256),
    random(0, 256));
}

// Theme names (for status bar / bottom bar)
const char *themeNames[] = {
  "Night", "Morning", "Day", "Evening",
  "Neon", "Sunset", "Arctic", "Terminal",
  "Custom1", "Custom2",
  "CyberBlue", "RetroAmber", "Vaporwave", "Matrix",
  "DeepSea", "SolarStorm", "IceFire", "Synthwave"
};

// Popup 1 labels (first theme menu)
const char *popup1Names[] = {
  "Night", "Morning", "Day", "Evening",
  "Neon", "Sunset", "Arctic", "Terminal",
  "Custom1", "More..."
};

// Popup 2 labels (second theme menu)
const char *popup2Names[] = {
  "Custom2", "CyberBlue", "RetroAmber", "Vaporwave",
  "Matrix", "DeepSea", "SolarStorm", "IceFire",
  "Synthwave", "Back"
};

Theme themes[] = {
  //{ background, main time color, seconds/AMPM color, date text color, accent color }
  { RGB(5, 10, 25), RGB(0, 255, 200), RGB(255, 80, 120), RGB(160, 200, 255), RGB(0, 120, 255) },      // 0 Night
  { RGB(15, 30, 60), RGB(255, 255, 255), RGB(255, 200, 80), RGB(200, 230, 255), RGB(80, 180, 255) },  // 1 Morning
  { RGB(10, 25, 45), RGB(0, 255, 180), RGB(255, 120, 80), RGB(180, 220, 255), RGB(0, 150, 255) },     // 2 Day
  { RGB(5, 8, 20), ORANGE, ARDUINO_GREEN, RGB(200, 200, 255), RGB(120, 80, 255) },                    // 3 Evening
  { RGB(0, 0, 0), RGB(0, 255, 180), RGB(255, 0, 120), RGB(0, 200, 255), RGB(0, 255, 255) },           // 4 Neon
  { RGB(30, 10, 5), RGB(255, 120, 40), RGB(255, 80, 60), RGB(255, 200, 150), RGB(255, 120, 80) },     // 5 Sunset
  { RGB(5, 20, 40), RGB(180, 255, 255), RGB(10, 200, 255), RGB(200, 230, 255), RGB(80, 180, 255) },   // 6 Arctic
  { RGB(0, 0, 0), RGB(0, 255, 0), RGB(0, 180, 0), RGB(0, 255, 120), RGB(0, 255, 0) },                 // 7 Terminal
  { DKGREEN, LTGREY, RED, AZURE, COLOR_CYAN },                                                        // 8 Custom1 Use any of the Human Readable colors from above.
  { RGB(15, 30, 60), ORANGE, ARDUINO_GREEN, AZURE, ORANGE },                                          // 9 Custom2
  { RGB(0, 0, 40), RGB(0, 180, 255), RGB(255, 80, 200), RGB(180, 220, 255), RGB(0, 120, 255) },       // 10 CyberBlue
  { RGB(10, 5, 0), RGB(255, 180, 40), RGB(255, 80, 40), RGB(255, 200, 150), RGB(255, 120, 40) },      // 11 RetroAmber
  { RGB(20, 0, 40), RGB(255, 120, 255), RGB(0, 255, 200), RGB(255, 200, 255), RGB(120, 80, 255) },    // 12 Vaporwave
  { RGB(0, 0, 0), RGB(0, 255, 0), RGB(0, 180, 0), RGB(0, 255, 120), RGB(0, 120, 0) },                 // 13 Matrix
  { RGB(0, 10, 30), RGB(0, 200, 255), RGB(0, 120, 200), RGB(180, 220, 255), RGB(0, 80, 160) },        // 14 DeepSea
  { RGB(30, 5, 0), RGB(255, 200, 80), RGB(255, 80, 0), RGB(255, 220, 160), RGB(255, 140, 40) },       // 15 SolarStorm
  { RGB(5, 10, 25), RGB(180, 255, 255), RGB(255, 80, 60), RGB(200, 230, 255), RGB(0, 200, 255) },     // 16 IceFire
  { RGB(10, 0, 30), RGB(255, 120, 200), RGB(0, 255, 180), RGB(255, 200, 255), RGB(120, 80, 255) }     // 17 Synthwave
};

int currentThemeIndex = 0;

// ---------- Forward declarations ----------
void drawHUDFrame();
void displayClock();
void drawPopup();
void drawPopup2();
void drawSettingsPanel();
void hidePopup();
void handleTouch();
int selectThemeIndex(int hour);

// ---------- NTP ----------
unsigned long getNTP() {
  unsigned long epoch = WiFi.getTime();
  if (epoch < 1700000000UL) return 0;
  return epoch;
}

void syncTimeFromNTP() {
  unsigned long epoch = 0;
  for (int i = 0; i < 10; i++) {
    epoch = getNTP();
    if (epoch != 0) break;

    tft.fillScreen(DKBLUE);
    tft.setFont(&Orbitron_VariableFont_wght24pt7b);
    tft.setTextColor(OLIVE);
    tft.setTextSize(1);
    tft.setCursor(16, 42);
    tft.print("RUSSCLOX");
    tft.setFont();
    tft.setCursor(18, 52);
    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.print("Diagnostics");
    tft.setTextColor(CHARTRUSE);
    tft.setTextSize(2);
    tft.setCursor(18, 82);
    tft.print("Connecting to WiFi: ");
    tft.setTextColor(YELLOW);
    tft.print(ssid);
    delay(1500);
  }

  if (epoch == 0) {
    tft.fillScreen(DKRED);
    tft.setTextColor(COLOR_MAGENTA);
    tft.setTextSize(2);
    tft.setCursor(16, 10);
    tft.println("FAILED");
    tft.setTextSize(3);
    tft.setTextColor(WHITE);
    tft.println("         Retrying...");
    delay(1500);
    syncTimeFromNTP();
  }

  epoch += TZ_OFFSET;
  RTCTime t(epoch);
  RTC.setTime(t);
}

// ---------- WiFi Indicator ----------
void drawWiFiIndicator() {
  long rssi = WiFi.RSSI();
  int bars = 0;
  if (rssi > -50) bars = 4;
  else if (rssi > -60) bars = 3;
  else if (rssi > -70) bars = 2;
  else if (rssi > -80) bars = 1;

  int baseX = 440;
  int baseY = 10;
  tft.fillRect(baseX - 10, baseY - 2, 40, 20, RGB(5, 15, 35));

  for (int i = 0; i < 4; i++) {
    int h = (i + 1) * 4;
    uint16_t color = (i < bars) ? RGB(0, 255, 200) : RGB(40, 80, 120);
    tft.fillRect(baseX + i * 6, baseY + (16 - h), 4, h, color);
  }
}

// ---------- HUD Frame ----------
void drawHUDFrame() {
  Theme &th = themes[currentThemeIndex];
  tft.fillScreen(th.bg);

  // Top bar
  tft.fillRect(0, 0, 480, 40, RGB(5, 15, 35));
  tft.drawRect(0, 0, 480, 40, th.accent);

  tft.setFont(&Orbitron_VariableFont_wght12pt7b);
  tft.setTextColor(th.timeColor);
  tft.setTextSize(1);
  tft.setCursor(12, 28);
  tft.print("RUSSCLOX v");
  tft.print(VERSION);

  tft.setFont();
  tft.setTextColor(th.dateColor);
  tft.setTextSize(1);
  tft.setCursor(240, 16);
  tft.print("UNO R4 WiFi  |  NTP SYNC");

  // Bottom bar
  tft.fillRect(0, 280, 480, 40, RGB(5, 15, 35));
  tft.drawRect(0, 280, 480, 40, th.accent);

  tft.setTextColor(th.accent);
  tft.setTextSize(2);
  tft.setCursor(20, 292);
  tft.print("[THEME]");

  tft.setCursor(200, 292);
  tft.print(themeNames[currentThemeIndex]);

  if (setting_autoTheme == true) {
    tft.setTextColor(th.accent);
    tft.setTextSize(2);
    tft.setCursor(390, 292);
    tft.print("*AUTO*");
  }

  // Center frame
  tft.drawRoundRect(40, 60, 400, 200, 16, th.accent);
  tft.drawRoundRect(44, 64, 392, 192, 16, th.accent);
}

// ---------- Popup Menu 1 ----------
void drawPopup() {
  popupMode = POPUP_THEME;
  popupVisible = true;
  Theme &th = themes[currentThemeIndex];

  tft.fillRect(20, 80, 440, 200, th.bg);
  tft.drawRect(20, 80, 440, 200, th.accent);
  tft.setTextSize(2);
  tft.setTextColor(th.timeColor);

  int idx = 0;
  for (int row = 0; row < 5; row++) {
    for (int col = 0; col < 2; col++) {
      int x = 40 + col * 200;
      int y = 100 + row * 40;
      tft.setCursor(x, y);
      tft.print(popup1Names[idx++]);
    }
  }

  // Settings button
  tft.fillRect(20, 290, 200, 30, th.bg);
  tft.drawRect(20, 290, 200, 30, th.accent);
  tft.setCursor(40, 298);
  tft.print("Settings");
}

// ---------- Popup Menu 2 ----------
void drawPopup2() {
  popupMode = POPUP_THEME2;
  popupVisible = true;
  Theme &th = themes[currentThemeIndex];

  tft.fillRect(20, 80, 440, 200, th.bg);
  tft.drawRect(20, 80, 440, 200, th.accent);
  tft.setTextSize(2);
  tft.setTextColor(th.timeColor);

  int idx = 0;
  for (int row = 0; row < 5; row++) {
    for (int col = 0; col < 2; col++) {
      int x = 40 + col * 200;
      int y = 100 + row * 40;
      tft.setCursor(x, y);
      tft.print(popup2Names[idx++]);
    }
  }
}

// ---------- Settings Panel ----------
void drawSettingsPanel() {
  popupMode = POPUP_SETTINGS;
  popupVisible = true;
  Theme &th = themes[currentThemeIndex];

  displayClock();
  tft.fillRect(40, 60, 400, 220, th.bg);
  tft.drawRect(40, 60, 400, 220, th.accent);

  tft.setTextSize(2);
  tft.setTextColor(th.timeColor);
  tft.setCursor(60, 80);
  tft.print("Settings");

  const char *labels[] = {
    "24-hour time",
    "Show seconds",
    "Auto theme",
    "WiFi indicator"
  };
  bool values[] = {
    setting_24h,
    setting_showSeconds,
    setting_autoTheme,
    setting_showWiFi
  };

  for (int i = 0; i < 4; i++) {
    int y = 120 + i * 40;
    tft.drawRect(60, y, 20, 20, th.accent);
    if (values[i]) {
      tft.fillRect(62, y + 2, 16, 16, th.accent);
    }
    tft.setCursor(100, y);
    tft.print(labels[i]);
  }

  // Back button
  tft.fillRect(60, 280, 120, 30, th.bg);
  tft.drawRect(60, 280, 120, 30, th.accent);
  tft.setCursor(80, 288);
  tft.print("Back");
}

/*|------------------------------------------------------------------|
  |--------------------------- About Box ----------------------------|
  |------------------------------------------------------------------|*/
void aboutBox() {
  popupMode = ABOUT_BOX;
  popupVisible = true;
  Theme &th = themes[currentThemeIndex];
  tft.setFont(&Orbitron_VariableFont_wght24pt7b);
  tft.fillRect(20, 40, 440, 240, RGB(5, 15, 35));
  tft.drawRect(20, 40, 440, 240, th.accent);
  tft.setTextSize(1);
  tft.setTextColor(th.timeColor);
  tft.setCursor(40, 79);
  tft.print("ABOUT");
  tft.setFont(&Orbitron_VariableFont_wght12pt7b);
  tft.setTextColor(OLIVE);
  tft.setTextSize(2);
  tft.setCursor(40, 121);
  // tft.setTextColor(th.accent);
  tft.print("RUSSCLOX");
  tft.setTextSize(1);
  tft.setTextColor(th.dateColor);
  tft.setCursor(40, 148);
  tft.print("AUTO-THEME Digital Clock");
  tft.setFont();
  tft.setTextSize(2);
  tft.setTextColor(th.timeColor);
  tft.setCursor(40, 165);
  tft.print("Automatic theme setting based on");
  tft.setCursor(40, 195);
  tft.println("the time of day. Tap [THEME] to");
  tft.setCursor(40, 225);
  tft.println("manually choose from 18 themes!");
  tft.setTextColor(YELLOW);
  tft.setCursor(315, 260);
  tft.print("RUSSCLOX v");
  tft.setTextColor(WHITE);
  tft.print(VERSION);

  // Exit Button
  tft.fillRect(400, 40, 60, 50, th.bg);
  tft.drawRect(400, 40, 60, 50, th.accent);
  tft.setFont(&Orbitron_VariableFont_wght12pt7b);
  tft.setTextColor(COLOR_RED);
  tft.setTextSize(2);
  tft.setCursor(411, 79);
  tft.print("X");
  tft.setFont();
}

void hidePopup() {
  popupVisible = false;
  popupMode = POPUP_NONE;
  drawHUDFrame();
  displayClock();
}

// ---------- Automatic Theme selection times ----------
int selectThemeIndex(int hour) {
  if (hour <= 5) return 4;
  if (hour <= 10) return 1;
  if (hour <= 14) return 2;
  if (hour <= 16) return 11;
  if (hour <= 18) return 15;
  if (hour <= 20) return 12;
  return 13;
}

// ---------- Clock ----------
void displayClock() {
  RTC.getTime(currentTime);
  int hour = currentTime.getHour();
  int minute = currentTime.getMinutes();
  int second = currentTime.getSeconds();
  int day = currentTime.getDayOfMonth();
  int month = (int)currentTime.getMonth() + 1;
  int year = currentTime.getYear();
  int weekday = (int)currentTime.getDayOfWeek();

  if (setting_autoTheme) {
    int newTheme = selectThemeIndex(hour);
    if (newTheme != currentThemeIndex) {
      currentThemeIndex = newTheme;
      drawHUDFrame();
    }
  }

  Theme &th = themes[currentThemeIndex];

  char timeBuffer[32];
  char secBuffer[8];
  const char *ampm = (hour >= 12) ? "PM" : "AM";

  if (setting_24h) {
    sprintf(timeBuffer, "%02d %02d", hour, minute);
  } else {
    int h12 = (hour % 12 == 0) ? 12 : (hour % 12);
    sprintf(timeBuffer, "%2d %02d", h12, minute);
  }
  sprintf(secBuffer, "%02d", second);

  char dateBuffer[64];
  sprintf(dateBuffer, "%s  %s %d, %d",
          (const char *[]){ "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" }[weekday],
          (const char *[]){ "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" }[month - 1],
          day, year);


  if (!setting_showWiFi) {
    tft.setCursor(390, 15);
    tft.setTextSize(2);
    tft.setTextColor(RGB(0, 0, 220));
    tft.print("[ABOUT]");
  }

  if (setting_showWiFi) drawWiFiIndicator();
  if (setting_showWiFi) {
    tft.setTextColor(YELLOW, RGB(5, 15, 35));
    tft.setTextSize(1);
    tft.setCursor(428, 29);
    tft.print("rssi:");
    tft.setTextColor(COLOR_WHITE, RGB(5, 15, 35));
    tft.print(WiFi.RSSI());
  }

  // ----- BUILD TIME STRING -----
  bool colonOn = (second % 2 == 0);

  if (setting_24h) {
    sprintf(timeBuffer, colonOn ? "%02d:%02d" : "%02d %02d", hour, minute);
  } else {
    int h12 = (hour % 12 == 0) ? 12 : (hour % 12);
    sprintf(timeBuffer, colonOn ? "%2d:%02d" : "%2d %02d", h12, minute);
  }

  // ----- DRAW TIME -----
  tft.setFont(&Orbitron_VariableFont_wght24pt7b);
  tft.setTextSize(2);
  tft.setTextColor(th.timeColor);
  tft.setCursor(80, 185);
  tft.fillRect(55, 110, 370, 90, th.bg);
  tft.print(timeBuffer);
  tft.setFont();

  if (!setting_24h) {
    tft.setTextColor(th.secColor, th.bg);
    tft.setTextSize(3);
    tft.setCursor(390, 155);
    tft.print(ampm);
  }

  if (setting_showSeconds) {
    tft.setTextColor(th.secColor, th.bg);
    tft.setTextSize(3);
    tft.setCursor(385, 80);
    tft.print(secBuffer);
  }

  tft.setTextColor(th.dateColor, th.bg);
  tft.setTextSize(3);
  tft.setCursor(80, 210);
  tft.print(dateBuffer);
}

// ---------- Touch ----------
void handleTouch() {
  static bool wasTouched = false;
  int x, y;
  bool touched = tft.getTouch(x, y);

  // --- NO POPUP ---
  if (popupMode == POPUP_NONE) {
    if (touched && !wasTouched && y >= 280 && y <= 319 && x >= 0 && x <= 160) {
      drawPopup();
      wasTouched = touched;
      return;
    }
  }

  // --- THEME POPUP 1 ---
  else if (popupMode == POPUP_THEME) {
    if (touched) {
      // SETTINGS button
      if (x >= 20 && x <= 220 && y >= 290 && y <= 320) {
        drawSettingsPanel();
        wasTouched = touched;
        return;
      }

      // LEFT COLUMN
      if (x >= 40 && x <= 200) {
        if (y >= 100 && y < 140) currentThemeIndex = 0;       // Night
        else if (y >= 140 && y < 180) currentThemeIndex = 2;  // Day
        else if (y >= 180 && y < 220) currentThemeIndex = 4;  // Neon
        else if (y >= 220 && y < 260) currentThemeIndex = 6;  // Arctic
        else if (y >= 260 && y < 300) currentThemeIndex = 8;  // Custom1
        else goto skipTheme1;

        setting_autoTheme = false;
        hidePopup();
        wasTouched = touched;
        return;
      }

      // RIGHT COLUMN
      if (x >= 240 && x <= 400) {
        if (y >= 100 && y < 140) currentThemeIndex = 1;       // Morning
        else if (y >= 140 && y < 180) currentThemeIndex = 3;  // Evening
        else if (y >= 180 && y < 220) currentThemeIndex = 5;  // Sunset
        else if (y >= 220 && y < 260) currentThemeIndex = 7;  // Terminal
        else if (y >= 260 && y < 300) {
          drawPopup2();  // More...
          wasTouched = touched;
          return;
        } else goto skipTheme1;

        setting_autoTheme = false;
        hidePopup();
        wasTouched = touched;
        return;
      }

skipTheme1:;
    }

    wasTouched = touched;
    return;
  }

  // --- SETTINGS POPUP ---
  else if (popupMode == POPUP_SETTINGS) {
    if (touched) {
      if (x >= 60 && x <= 80) {
        if (y >= 120 && y < 140) setting_24h = !setting_24h;
        else if (y >= 160 && y < 180) setting_showSeconds = !setting_showSeconds;
        else if (y >= 200 && y < 220) setting_autoTheme = !setting_autoTheme;
        else if (y >= 240 && y < 260) setting_showWiFi = !setting_showWiFi;
        else goto skipSettings;

        drawSettingsPanel();
        wasTouched = touched;
        return;
      }

      if (x >= 60 && x <= 180 && y >= 280 && y <= 310) {
        hidePopup();
        wasTouched = touched;
        return;
      }

skipSettings:;
    }

    wasTouched = touched;
    return;
  }

  // --- THEME POPUP 2 (extra themes) ---
  else if (popupMode == POPUP_THEME2) {
    if (touched) {

      // LEFT COLUMN
      if (x >= 40 && x <= 200) {
        if (y >= 100 && y < 140) currentThemeIndex = 9;        // Custom2
        else if (y >= 140 && y < 180) currentThemeIndex = 11;  // CyberBlue
        else if (y >= 180 && y < 220) currentThemeIndex = 13;  // RetroAmber
        else if (y >= 220 && y < 260) currentThemeIndex = 15;  // Vaporwave
        else if (y >= 260 && y < 300) currentThemeIndex = 17;  // Matrix
        else goto skipTheme2;

        setting_autoTheme = false;
        hidePopup();
        wasTouched = touched;
        return;
      }

      // RIGHT COLUMN
      if (x >= 240 && x <= 400) {
        if (y >= 100 && y < 140) currentThemeIndex = 10;       // DeepSea
        else if (y >= 140 && y < 180) currentThemeIndex = 12;  // SolarStorm
        else if (y >= 180 && y < 220) currentThemeIndex = 14;  // IceFire
        else if (y >= 220 && y < 260) currentThemeIndex = 16;  // Synthwave
        else if (y >= 260 && y < 300) {
          drawPopup();  // Back
          wasTouched = touched;
          return;
        } else goto skipTheme2;

        setting_autoTheme = false;
        hidePopup();
        wasTouched = touched;
        return;
      }

skipTheme2:;
    }

    wasTouched = touched;
    return;
  }

  /*|------------------------------------------------------------------|
  |---------------------- ABOUT BOX TOUCHY SPOTS --------------------|
  |------------------------------------------------------------------|*/
  if (x >= 300 && x <= 460 && y >= 0 && y <= 50) {
    aboutBox();
    wasTouched = touched;
    return;
  }
  if (popupMode == ABOUT_BOX) {

    if (x >= 400 && x <= 460 && y >= 50 && y <= 100) {
      hidePopup();
      wasTouched = touched;
      return;
    }
  }

  wasTouched = touched;
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);
  tft.begin();
  tft.setRotation(1);
  RTC.begin();
  WiFi.begin(ssid, pass);
  delay(2000);
  syncTimeFromNTP();
  drawHUDFrame();
  displayClock();
}

// ---------- Loop ----------
void loop() {
  handleTouch();

  unsigned long now = millis();
  if (!popupVisible && now - lastMillis >= 1000) {
    lastMillis = now;
    displayClock();
  }

  if (now - lastNTPSync >= ntpInterval) {
    lastNTPSync = now;
    syncTimeFromNTP();
  }
}
