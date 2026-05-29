/*
  ┌───────────────────────────────────────────────────────────────────┐
  |        RUSSCLOX v2 — UNO R4 WiFi + DIYables 3.5" ILI9486          |
  |         - Futuristic HUD style                                    |
  |         - Centered time layout                                    |
  |         - Dynamic themes                                          |
  |         - Popup theme menu                                        |
  |         - Settings menu with checkboxes                           |
  |         - Resistive touch (4-wire)                                |
  |         - WiFi NTP + RTC                                          |
  |───────────────────────────────────────────────────────────────────|
  |                    😎 Russ McEwen 5/13/26                         |
  |                   Posted to GitHub 5/28/26                        |
  |───────────────────────────────────────────────────────────────────|
  └───────────────────────────────────────────────────────────────────┘*/
#include <DIYables_TFT_Touch_Shield.h>
#include <WiFiS3.h>
#include <RTC.h>
// ---------- TFT ----------
DIYables_TFT_RM68140_Shield tft;
// ---------- WiFi ----------
char ssid[] = "FWA_XC436L";
char pass[] = "silk4-sky-store";
// ---------- Time / RTC ----------
RTCTime currentTime;
unsigned long lastMillis = 0;
unsigned long lastNTPSync = 0;
const unsigned long ntpInterval = 2UL * 60UL * 1000UL;  // 5 minutes
const long TZ_OFFSET = -4L * 3600L;                     // EDT
// ---------- Popup ----------
bool popupVisible = false;
enum PopupMode {
  POPUP_NONE,
  POPUP_THEME,
  POPUP_SETTINGS
};
PopupMode popupMode = POPUP_NONE;
// ---------- Color helper ----------
#define RGB(r, g, b) DIYables_TFT::colorRGB(r, g, b)
// #define RGB(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))
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
#define RED RGB(255, 0, 0)
#define GREEN RGB(0, 255, 0)
#define YELLOW RGB(255, 255, 0)
#define WHITE RGB(255, 255, 255)

// ----------------------------------------------|
// SECTION 4: RANDOM COLOR UTILITY --------------|
// ----------------------------------------------|
uint16_t randColor() {
  return RGB(
    random(0, 256),
    random(0, 256),
    random(0, 256));
}

const char *names[] = {
  "Night", "Morning", "Day", "Evening",
  "Neon", "Sunset", "Arctic", "Terminal", "Custom1", "Custom2"
};

Theme themes[] = {
  // { background,   main time color,   seconds/AMPM color,   date text color,   accent color }
  // 0 — NIGHT THEME
  // Dark navy background, cyan time, magenta seconds, soft blue date, bright blue accents
  { RGB(5, 10, 25), RGB(0, 255, 200), RGB(255, 80, 120), RGB(160, 200, 255), RGB(0, 120, 255) },
  // 1 — MORNING THEME
  // Soft dawn blue background, white time, warm amber seconds, pale sky date, light blue accents
  { RGB(15, 30, 60), RGB(255, 255, 255), RGB(255, 200, 80), RGB(200, 230, 255), RGB(80, 180, 255) },
  // 2 — DAY THEME
  // Bright teal HUD background, aqua time, orange seconds, bright sky date, medium blue accents
  { RGB(10, 25, 45), RGB(0, 255, 180), RGB(255, 120, 80), RGB(180, 220, 255), RGB(0, 150, 255) },
  // 3 — EVENING THEME
  // Deep purple‑tinted dusk background, purple time, magenta seconds, soft lavender date, violet accents
  { RGB(5, 8, 20), ORANGE, ARDUINO_GREEN, RGB(200, 200, 255), RGB(120, 80, 255) },
  // 4 — NEON THEME
  // Pure black cyberpunk background, teal time, hot‑pink seconds, neon cyan date, bright cyan accents
  { RGB(0, 0, 0), RGB(0, 255, 180), RGB(255, 0, 120), RGB(0, 200, 255), RGB(0, 255, 255) },
  // 5 — SUNSET THEME
  // Warm orange/red background, orange time, red seconds, peach date, orange accents
  { RGB(30, 10, 5), RGB(255, 120, 40), RGB(255, 80, 60), RGB(255, 200, 150), RGB(255, 120, 80) },
  // 6 — ARCTIC THEME
  // Cold blue background, icy white time, bright blue seconds, pale ice date, blue accents
  { RGB(5, 20, 40), RGB(180, 255, 255), RGB(10, 200, 255), RGB(200, 230, 255), RGB(80, 180, 255) },
  // 7 — TERMINAL GREEN THEME
  // Black CRT background, bright green time, dark green seconds, mint date, green accents
  { RGB(0, 0, 0), RGB(0, 255, 0), RGB(0, 180, 0), RGB(0, 255, 120), RGB(0, 255, 0) },
  // 8 — CUSTOM1 THEME
  { randColor(), randColor(), randColor(), randColor(), randColor() },
  // 9 — CUSTOM2 THEME
  { RGB(15, 30, 60), BROWN, ARDUINO_GREEN, AZURE, ORANGE }
};
int currentThemeIndex = 0;
// ---------- Forward declarations ----------
void drawHUDFrame();
void displayClock();
void drawPopup();
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
    tft.setTextColor(OLIVE);
    tft.setTextSize(4);
    tft.setCursor(16, 12);
    tft.print("RUSSCLOX");
    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.print(" Diagnostics");
    tft.setTextColor(CHARTRUSE);
    tft.setTextSize(2);
    tft.setCursor(16, 52);
    //   tft.print("Connecting WIFI...");
    // tft.setTextSize(1);
    // tft.setCursor(40, 30);
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
  tft.setTextColor(th.timeColor);
  tft.setTextSize(2);
  tft.setCursor(16, 12);
  tft.print("RUSSCLOX v2");
  tft.setTextColor(th.dateColor);
  tft.setTextSize(1);
  tft.setCursor(200, 16);
  tft.print("UNO R4 WiFi  |  NTP SYNC");
  // Bottom bar
  tft.fillRect(0, 280, 480, 40, RGB(5, 15, 35));
  tft.drawRect(0, 280, 480, 40, th.accent);
  tft.setTextColor(th.accent);
  tft.setTextSize(2);
  tft.setCursor(20, 292);
  tft.print("[THEME]");
  tft.setCursor(200, 292);
  tft.print(names[currentThemeIndex]);
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
// ---------- Popup Menu ----------
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
      tft.print(names[idx++]);
    }
  }
  // Settings button
  tft.fillRect(20, 290, 200, 30, th.bg);
  tft.drawRect(20, 290, 200, 30, th.accent);
  tft.setCursor(40, 298);
  tft.print("Settings");
}
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
void hidePopup() {
  popupVisible = false;
  popupMode = POPUP_NONE;
  drawHUDFrame();
  displayClock();
}
// ---------- Theme selection ----------
int selectThemeIndex(int hour) {
  if (hour <= 5) return 0;
  if (hour <= 11) return 1;
  if (hour <= 17) return 9;
  if (hour <= 20) return 5;
  return 3;
}
// 0 — NIGHT THEME
// 1 — MORNING THEME
// 2 — DAY THEME
// 3 — EVENING THEME
// 4 — NEON THEME
// 5 — SUNSET THEME
// 6 — ARCTIC THEME
// 7 — TERMINAL GREEN THEME
// 8 — CUSTOM1 THEME
// 9 — CUSTOM2 THEME
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
  if (setting_showWiFi) drawWiFiIndicator();
  if (setting_showWiFi) {
    tft.setTextColor(COLOR_WHITE, RGB(5, 15, 35));
    tft.setTextSize(1);
    tft.setCursor(442, 29);
    tft.print(WiFi.RSSI());
  }
  tft.setTextColor(th.timeColor, th.bg);
  tft.setTextSize(10);
  tft.setCursor(78, 105);
  tft.print(timeBuffer);
  bool colonOn = (second % 2 == 0);
  int colonX = 100 + 95;
  int colonY = 105;
  if (colonOn) {
    tft.setCursor(colonX, colonY);
    tft.print(":");
  } else {
    tft.fillRect(colonX, colonY, 30, 60, th.bg);
  }
  if (!setting_24h) {
    tft.setTextColor(th.secColor, th.bg);
    tft.setTextSize(3);
    tft.setCursor(390, 155);
    tft.print(ampm);
  } else {
    // tft.fillRect(360, 150, 80, 30, th.bg);
  }
  if (setting_showSeconds) {
    tft.setTextColor(th.secColor, th.bg);
    tft.setTextSize(3);
    tft.setCursor(385, 80);
    tft.print(secBuffer);
  } else {
    // tft.fillRect(360, 100, 80, 40, th.bg);
  }
  tft.setTextColor(th.dateColor, th.bg);
  tft.setTextSize(3);
  tft.setCursor(70, 210);
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
  // --- THEME POPUP ---
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
        if (y >= 100 && y < 140) currentThemeIndex = 0;
        else if (y >= 140 && y < 180) currentThemeIndex = 2;
        else if (y >= 180 && y < 220) currentThemeIndex = 4;
        else if (y >= 220 && y < 260) currentThemeIndex = 6;
        else if (y >= 260 && y < 300) currentThemeIndex = 8;
        else goto skipTheme;
        setting_autoTheme = false;
        hidePopup();
        wasTouched = touched;
        return;
      }
      // RIGHT COLUMN
      if (x >= 240 && x <= 400) {
        if (y >= 100 && y < 140) currentThemeIndex = 1;
        else if (y >= 140 && y < 180) currentThemeIndex = 3;
        else if (y >= 180 && y < 220) currentThemeIndex = 5;
        else if (y >= 220 && y < 260) currentThemeIndex = 7;
        else if (y >= 260 && y < 300) currentThemeIndex = 9;
        else goto skipTheme;
        setting_autoTheme = false;
        hidePopup();
        wasTouched = touched;
        return;
      }
skipTheme:;
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
