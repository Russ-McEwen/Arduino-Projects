// ------------------------------------------------------------
// RUSSCLOX — UNO R4 WiFi
// Optimized Architecture (Clean Modern Style)
// ------------------------------------------------------------

#include <DIYables_TFT_Shield.h>
#include <WiFiS3.h>
#include <RTC.h>
#include "arduino_secrets.h"

// ------------------------------------------------------------
// Display + RTC
// ------------------------------------------------------------
DIYables_TFT_ILI9486_Shield tft;
RTCTime now;

// ------------------------------------------------------------
// WiFi Networks
// ------------------------------------------------------------
struct WiFiNetwork {
  const char* ssid;
  const char* pass;
};

WiFiNetwork wifiList[] = {
  { SECRET_SSID_1, SECRET_PASS_1 },
  { SECRET_SSID_2, SECRET_PASS_2 }
};

const int WIFI_COUNT = sizeof(wifiList) / sizeof(wifiList[0]);

// ------------------------------------------------------------
// Geometry
// ------------------------------------------------------------
static const int16_t CX = 240;
static const int16_t CY = 160;
static const int16_t R = 140;

// ------------------------------------------------------------
// Colors
// ------------------------------------------------------------
#define RGB(r, g, b) DIYables_TFT::colorRGB(r, g, b)

static const uint16_t COL_BG = RGB(70, 0, 0);
static const uint16_t COL_FACE = RGB(0, 0, 65);
static const uint16_t COL_HOUR = RGB(180, 180, 180);
static const uint16_t COL_MIN = RGB(255, 255, 255);
static const uint16_t COL_SEC = RGB(255, 0, 0);
static const uint16_t COL_TICK = RGB(127, 127, 127);
static const uint16_t COL_NUM = RGB(0, 90, 90);
static const uint16_t COL_DIGITAL = RGB(100, 255, 50);
static const uint16_t COL_LABEL = RGB(200, 150, 80);

// ------------------------------------------------------------
// Hand State
// ------------------------------------------------------------
struct HandState {
  int16_t hx, hy;
  int16_t mx, my;
  int16_t sx, sy;
};

HandState lastHand = { 0, 0, 0, 0, 0, 0 };

// ------------------------------------------------------------
// Helpers
// ------------------------------------------------------------
inline float deg2rad(float d) {
  return d * 0.01745329252f;
}

// ------------------------------------------------------------
// WiFi connect helper
// ------------------------------------------------------------
bool tryConnect(const char* ssid, const char* pass, int timeoutMs = 8000) {
  WiFi.begin(ssid, pass);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeoutMs) {
    delay(250);
  }

  return WiFi.status() == WL_CONNECTED;
}

void autoConnectWiFi() {
  Serial.println("WiFi: auto connect...");
  for (int i = 0; i < WIFI_COUNT; i++) {
    Serial.print("  trying ");
    Serial.println(wifiList[i].ssid);
    if (tryConnect(wifiList[i].ssid, wifiList[i].pass)) {
      Serial.print("  connected to ");
      Serial.println(wifiList[i].ssid);
      return;
    }
  }
  Serial.println("  no WiFi networks available");
}

void drawDial() {
  tft.fillScreen(COL_BG);

  // Outer rings
  for (int i = 0; i <= 8; i += 4)
    tft.drawCircle(CX, CY, R + i, RGB(197, 194, 197));

  for (int i = 2; i <= 8; i += 4)
    tft.drawCircle(CX, CY, R + i, RGB(54, 54, 54));

  // Face
  tft.fillCircle(CX, CY, R + 2, COL_FACE);

  // Hour ticks
  for (int h = 0; h < 12; h++) {
    float a = deg2rad(h * 30 - 90);
    int16_t x1 = CX + (int16_t)((R - 20) * cos(a));
    int16_t y1 = CY + (int16_t)((R - 20) * sin(a));
    int16_t x2 = CX + (int16_t)(R * cos(a));
    int16_t y2 = CY + (int16_t)(R * sin(a));
    tft.drawLine(x1, y1, x2, y2, COL_TICK);
  }

  // Numbers
  tft.setTextColor(COL_LABEL);
  tft.setTextSize(2);
  tft.setCursor(CX - 48, CY - R + 65);
  tft.print("RUSSCLOX");

  tft.setTextColor(RGB(255, 255, 0));
  tft.setTextSize(4);
  tft.setCursor(CX - 19, CY - R + 26);
  tft.print("12");

  tft.setTextColor(COL_NUM);
  tft.setTextSize(3);
  tft.setCursor(CX - R + 25, CY - 10);
  tft.print("9");
  tft.setCursor(CX + R - 40, CY - 10);
  tft.print("3");
  tft.setCursor(CX - 10, CY + R - 45);
  tft.print("6");
}

HandState computeHands(float hours, float minutes, float seconds) {
  float minuteSmooth = minutes + (seconds / 60.0f);
  float hourSmooth = fmod(hours, 12.0f) + (minuteSmooth / 60.0f);

  float angH = deg2rad(hourSmooth * 30.0f - 90.0f);
  float angM = deg2rad(minuteSmooth * 6.0f - 90.0f);
  float angS = deg2rad(seconds * 6.0f - 90.0f);

  HandState h;
  h.hx = CX + (int16_t)((R - 70) * cos(angH));
  h.hy = CY + (int16_t)((R - 70) * sin(angH));

  h.mx = CX + (int16_t)((R - 30) * cos(angM));
  h.my = CY + (int16_t)((R - 30) * sin(angM));

  h.sx = CX + (int16_t)((R - 20) * cos(angS));
  h.sy = CY + (int16_t)((R - 20) * sin(angS));

  return h;
}

void eraseHands(const HandState& h) {
  // ------------------------------------------------------------
  // 1. Compute bounding box around old hands
  // ------------------------------------------------------------
  int16_t minX = min(min(h.hx, h.mx), h.sx) - 4;
  int16_t maxX = max(max(h.hx, h.mx), h.sx) + 4;
  int16_t minY = min(min(h.hy, h.my), h.sy) - 4;
  int16_t maxY = max(max(h.hy, h.my), h.sy) + 4;

  if (minX < 0) minX = 0;
  if (minY < 0) minY = 0;
  if (maxX > 479) maxX = 479;
  if (maxY > 319) maxY = 319;

  // ------------------------------------------------------------
  // 2. Reconstruct dial background inside bounding box
  // ------------------------------------------------------------
  for (int y = minY; y <= maxY; y++) {
    for (int x = minX; x <= maxX; x++) {

      int dx = x - CX;
      int dy = y - CY;
      float dist = sqrt(dx*dx + dy*dy);

      // OUTSIDE ALL RINGS → background
      if (dist > R + 8) {
        tft.drawPixel(x, y, COL_BG);
        continue;
      }

      // OUTER RINGS (exact radii)
      if (dist >= R && dist < R + 2) {
        tft.drawPixel(x, y, RGB(197,194,197)); // light ring
        continue;
      }
      if (dist >= R + 2 && dist < R + 4) {
        tft.drawPixel(x, y, RGB(54,54,54)); // dark ring
        continue;
      }
      if (dist >= R + 4 && dist < R + 6) {
        tft.drawPixel(x, y, RGB(197,194,197)); // light ring
        continue;
      }
      if (dist >= R + 6 && dist < R + 8) {
        tft.drawPixel(x, y, RGB(54,54,54)); // dark ring
        continue;
      }

      // FACE CIRCLE
      if (dist <= R + 2) {
        tft.drawPixel(x, y, COL_FACE);
      }

      // HOUR TICKS (12 lines)
      for (int t = 0; t < 12; t++) {
        float a = deg2rad(t * 30 - 90);
        int16_t x1 = CX + (int16_t)((R - 20) * cos(a));
        int16_t y1 = CY + (int16_t)((R - 20) * sin(a));
        int16_t x2 = CX + (int16_t)(R * cos(a));
        int16_t y2 = CY + (int16_t)(R * sin(a));

        float d = abs((y2 - y1) * x - (x2 - x1) * y + x2*y1 - y2*x1) /
                  sqrt((y2 - y1)*(y2 - y1) + (x2 - x1)*(x2 - x1));

        if (d < 1.2) {
          tft.drawPixel(x, y, COL_TICK);
        }
      }
    }
  }

  // ------------------------------------------------------------
  // 3. Redraw numbers + label (cheap)
  // ------------------------------------------------------------
  tft.setTextColor(COL_LABEL);
  tft.setTextSize(2);
  tft.setCursor(CX - 48, CY - R + 65);
  tft.print("RUSSCLOX");

  tft.setTextColor(RGB(255,255,0));
  tft.setTextSize(4);
  tft.setCursor(CX - 19, CY - R + 26);
  tft.print("12");

  tft.setTextColor(COL_NUM);
  tft.setTextSize(3);
  tft.setCursor(CX - R + 25, CY - 10);
  tft.print("9");
  tft.setCursor(CX + R - 40, CY - 10);
  tft.print("3");
  tft.setCursor(CX - 10, CY + R - 45);
  tft.print("6");
}

void drawHands(const HandState& h) {
  // Hour
  tft.drawLine(CX, CY, h.hx, h.hy, COL_HOUR);
  tft.drawTriangle(CX - 2, CY - 2, CX + 2, CY + 2, h.hx, h.hy, COL_HOUR);

  // Minute
  tft.drawLine(CX, CY, h.mx, h.my, COL_MIN);
  tft.drawTriangle(CX - 2, CY - 2, CX + 2, CY + 2, h.mx, h.my, COL_MIN);

  // Second
  tft.drawLine(CX, CY, h.sx, h.sy, COL_SEC);
  tft.drawTriangle(CX - 2, CY - 2, CX + 2, CY + 2, h.sx, h.sy, COL_SEC);

  // Center cap
  tft.fillCircle(CX, CY, 4, COL_SEC);
}

// ------------------------------------------------------------
// Setup
// ------------------------------------------------------------
void setup() {
  Serial.begin(115200);

  tft.begin();
  tft.setRotation(1);

  RTC.begin();

  autoConnectWiFi();

  if (WiFi.status() == WL_CONNECTED) {
    unsigned long epoch = WiFi.getTime();
    if (epoch > 1700000000UL) {
      RTCTime t(epoch);
      RTC.setTime(t);
    }
  }

  // Get initial time
  RTC.getTime(now);
  lastHand = { 0, 0, 0, 0, 0, 0 };

  // Draw static dial once
  drawDial();
}


void loop() {
  RTC.getTime(now);

  uint8_t hh = now.getHour();
  uint8_t mm = now.getMinutes();
  uint8_t ss = now.getSeconds();

  // Smooth seconds
  unsigned long ms = millis();
  float smoothSec = ss + (ms % 1000) / 1000.0f;

  // Digital time
  char buf[16];
  tft.setTextColor(COL_DIGITAL, COL_FACE);
  tft.setTextSize(3);
  tft.setCursor(CX - 55, CY + 62);
  sprintf(buf, "%02d:%02d", hh, mm);
  tft.print(buf);

  tft.setTextColor(COL_SEC, COL_FACE);
  tft.setTextSize(2);
  sprintf(buf, " %02d", ss);
  tft.print(buf);

  // Compute + draw hands
  HandState h = computeHands(hh, mm, smoothSec);
  eraseHands(lastHand);
  drawHands(h);
  lastHand = h;

  delay(16); // 60 FPS
}
