/*
   ----------------------------------------------------------
   |     RUSSCLOX Analog Edition — UNO R4 WiFi              |
   |   Huge 480×320 Analog Clockface with NTP + RTC         |
   |   Modernized colors, no touch, no buttons              |
   |   Rotation 1, radius 140                               |
   ----------------------------------------------------------
*/

#include <DIYables_TFT_Shield.h>
#include <TouchScreen.h>
#include <WiFiS3.h>
#include <RTC.h>

// ---------- Display ----------
DIYables_TFT_ILI9486_Shield tft;

// ---------------- WIFI + RTC ----------------
char ssid[] = "FWA_XC436L";
char pass[] = "silk4-sky-store";

RTCTime now;
unsigned long lastNTPSync = 0;
const unsigned long ntpInterval = 1UL * 60UL * 1000UL;  // 5 minutes
const long TZ_OFFSET = -4L * 3600L;                     // EDT

// ---------------- COLORS (Modernized) ----------------
#define RGB(r, g, b) DIYables_TFT::colorRGB(r, g, b)
/*---------------------------------------------|
 |   💩***** HUMAN READABLE COLORS! *****💩    |
 |--------------------------------------------*/
#define LTGREY RGB(180, 180, 180)
#define GREY RGB(127, 127, 127)
#define DARKGREY RGB(54, 54, 54)
#define TURQUOISE RGB(0, 128, 120)
#define PINK RGB(255, 128, 192)
#define OLIVE RGB(128, 128, 0)
#define PURPLE RGB(128, 0, 128)
#define AZURE RGB(0, 128, 255)
#define LTBLUE RGB(80, 200, 255)
#define ORANGE RGB(255, 128, 40)
#define WHITE RGB(255, 255, 255)
#define RED RGB(255, 0, 0)
#define GREEN RGB(0, 255, 0)
#define BLUE RGB(0, 0, 255)
#define YELLOW RGB(255, 255, 0)
#define BROWN RGB(130, 80, 40)
#define ARDUINO_GREEN RGB(0, 90, 90)
#define DKBLUE RGB(0, 0, 65)
#define DKGREEN RGB(0, 65, 0)
#define DKRED RGB(70, 0, 0)
#define DKBROWN RGB(110, 60, 20)
#define BLACK RGB(0, 0, 0)
#define DEEPPURPLE RGB(40, 0, 25)
#define BRONZE RGB(200, 150, 80)      // bronze
#define NEONRED RGB(255, 50, 40);     // neon red
#define NEONGREEN RGB(100, 255, 50);  // neon green

//      --------------------
// 💩   --- Clock Colors ---   💩
//      --------------------
uint16_t COL_BG = DKRED;
uint16_t COL_ACC = RGB(197, 194, 197);
uint16_t COL_HOUR = LTGREY;
uint16_t COL_MIN = WHITE;
uint16_t COL_SEC = RED;
uint16_t COL_DIAL = DARKGREY;
uint16_t COL_FACE = DKBLUE;
uint16_t COL_NUMBS = ARDUINO_GREEN;
uint16_t COL_DIGITAL = NEONGREEN;
uint16_t COL_TICKMK = GREY;
uint16_t COL_LABEL = BRONZE;

// uint16_t COL_BG = RGB(10, 10, 45);       // deep charcoal
// uint16_t COL_HOUR = RGB(180, 180, 180);  // steel grey
// uint16_t COL_MIN = RGB(230, 230, 230);   // bright silver
// uint16_t COL_SEC = RGB(255, 60, 60);     // neon red
// uint16_t COL_TICK = RGB(0, 200, 180);    // teal glow
// uint16_t COL_NUM = RGB(160, 200, 255);   // ice blue
// uint16_t COL_DIGITAL = RGB(255, 0, 0);   // white

// ---------------- GEOMETRY ----------------
int16_t cx = 240;
int16_t cy = 160;
int16_t radius = 140;

struct HandState {
  int16_t hx, hy;
  int16_t mx, my;
  int16_t sx, sy;
};
HandState lastHand = { 0, 0, 0, 0, 0, 0 };

// ---------------- TIME ----------------
uint8_t hh, mm, ss;
char buf[16];

// ---------------- HELPERS ----------------
float deg2rad(float d) {
  return d * 0.017453292519943295f;
}

// Draw simple WiFi icon + signal bars
void drawWiFiIcon(int x, int y, int strength) {
  uint16_t colOn = RGB(0, 255, 120);
  uint16_t colOff = RGB(60, 60, 60);

  // Dot
  tft.fillCircle(x, y + 12, 4, colOn);

  // First arc (outer)
  tft.drawCircle(x, y, 22, strength >= 3 ? colOn : colOff);

  // Second arc (middle)
  tft.drawCircle(x, y, 16, strength >= 2 ? colOn : colOff);

  // Third arc (inner)
  tft.drawCircle(x, y, 10, strength >= 1 ? colOn : colOff);

  // Signal bars (0–3)
  int bx = x + 30;
  int by = y + 20;

  tft.fillRect(bx, by - 4, 6, 4, strength >= 1 ? colOn : colOff);
  tft.fillRect(bx + 8, by - 8, 6, 8, strength >= 2 ? colOn : colOff);
  tft.fillRect(bx + 16, by - 12, 6, 12, strength >= 3 ? colOn : colOff);
}

void drawDial() {
  tft.fillScreen(COL_BG);
  // Outer ring

  for (int i = 0; i <= 8; i = i + 4)
    tft.drawCircle(cx, cy, radius + i, COL_ACC);

  for (int i = 2; i <= 8; i = i + 4)
    tft.drawCircle(cx, cy, radius + i, COL_DIAL);

  tft.fillCircle(cx, cy, radius + 2, COL_FACE);
  // Hour tick marks
  for (int h = 0; h < 12; h++) {
    float ang = deg2rad(h * 30.0f - 90.0f);
    int16_t x1 = cx + (int16_t)((radius - 20) * cos(ang));
    int16_t y1 = cy + (int16_t)((radius - 20) * sin(ang));
    int16_t x2 = cx + (int16_t)(radius * cos(ang));
    int16_t y2 = cy + (int16_t)(radius * sin(ang));
    tft.drawLine(x1, y1, x2, y2, COL_TICKMK);
  }
}

void eraseHands(const HandState& h) {
  tft.drawTriangle(cx - 2, cy - 2, cx + 2, cy + 2, h.hx, h.hy, COL_FACE);
  tft.drawLine(cx, cy, h.hx, h.hy, COL_FACE);
  tft.drawTriangle(cx - 2, cy - 2, cx + 2, cy + 2, h.mx, h.my, COL_FACE);
  tft.drawLine(cx, cy, h.mx, h.my, COL_FACE);
  tft.drawTriangle(cx - 2, cy - 2, cx + 2, cy + 2, h.sx, h.sy, COL_FACE);
  tft.drawLine(cx, cy, h.sx, h.sy, COL_FACE);
  tft.fillCircle(cx, cy, 4, COL_BG);
}

void drawHands(const HandState& h) {
  tft.drawLine(cx, cy, h.hx, h.hy, COL_HOUR);
  tft.drawTriangle(cx - 2, cy - 2, cx + 2, cy + 2, h.hx, h.hy, COL_HOUR);
  tft.drawLine(cx, cy, h.mx, h.my, COL_MIN);
  tft.drawTriangle(cx - 2, cy - 2, cx + 2, cy + 2, h.mx, h.my, COL_MIN);
  tft.drawLine(cx, cy, h.sx, h.sy, COL_SEC);
  tft.drawTriangle(cx - 2, cy - 2, cx + 2, cy + 2, h.sx, h.sy, COL_SEC);
  tft.fillCircle(cx, cy, 4, COL_SEC);
}


HandState computeHands(float hours, float minutes, float seconds) {
  float minuteSmooth = minutes + (seconds / 60.0f);
  float hourSmooth = fmod(hours, 12.0f) + (minuteSmooth / 60.0f);

  float angH = deg2rad(hourSmooth * 30.0f - 90.0f);
  float angM = deg2rad(minuteSmooth * 6.0f - 90.0f);
  float angS = deg2rad(seconds * 6.0f - 90.0f);

  HandState h;
  h.hx = cx + (int16_t)((radius - 70) * cos(angH));
  h.hy = cy + (int16_t)((radius - 70) * sin(angH));

  h.mx = cx + (int16_t)((radius - 30) * cos(angM));
  h.my = cy + (int16_t)((radius - 30) * sin(angM));

  h.sx = cx + (int16_t)((radius - 20) * cos(angS));
  h.sy = cy + (int16_t)((radius - 20) * sin(angS));

  return h;
}

// ---------------- NTP + RTC ----------------
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
    delay(500);
  }

  if (epoch == 0) return;

  epoch += TZ_OFFSET;
  RTCTime t(epoch);
  RTC.setTime(t);
}

void bootScreen(const char* ssid) {
  tft.fillScreen(DKBLUE);

  tft.setTextColor(BRONZE);
  tft.setTextSize(4);
  tft.setCursor(40, 40);
  tft.print("RUSSCLOX ");
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.print("Diagnostics");

  tft.setTextSize(2);
  tft.setCursor(40, 80);
  tft.print("Connecting to WiFi: ");
  tft.setTextColor(YELLOW);
  tft.print(ssid);

  // Connect WiFi
  int status = WL_IDLE_STATUS;
  while (status != WL_CONNECTED) {
    status = WiFi.begin(ssid, pass);
    delay(100);
  }

  // Draw WiFi icon + signal bars
  long rssi = WiFi.RSSI();
  int strength = 0;
  if (rssi > -70) strength = 3;
  else if (rssi > -80) strength = 2;
  else if (rssi > -90) strength = 1;

  drawWiFiIcon(60, 130, strength);

  tft.setCursor(40, 160);
  tft.setTextColor(RGB(0, 255, 120));
  tft.print("WiFi OK");

  delay(1000);

  // NTP sync
  tft.setCursor(40, 205);
  tft.setTextColor(RGB(255, 255, 0));
  tft.print("Syncing time...");

  unsigned long epoch = 0;
  for (int i = 0; i < 10; i++) {
    epoch = WiFi.getTime();
    if (epoch > 1700000000UL) break;
    delay(500);
  }

  if (epoch > 1700000000UL) {
    epoch += TZ_OFFSET;
    RTCTime t(epoch);
    RTC.setTime(t);

    tft.setCursor(40, 240);
    tft.setTextColor(RGB(0, 255, 120));
    tft.print("Time Set");
    tft.setTextColor(WHITE);
    tft.print(" Success!");
  } else {
    tft.setCursor(40, 240);
    tft.setTextColor(RGB(0, 255, 120));
    tft.print("Time Set");
    tft.setTextColor(RGB(255, 0, 0));
    tft.print(" NTP Failed");
    delay(500);
    bootScreen(ssid);
  }
  delay(1000);
}


// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);
  tft.begin();
  tft.setRotation(1);
  RTC.begin();
  // Boot screen handles WiFi + NTP
  bootScreen(ssid);

  // Now get time from RTC (already set)
  RTC.getTime(now);
  hh = now.getHour();
  mm = now.getMinutes();
  ss = now.getSeconds();

  drawDial();  // your working dial
}

// ---------------- LOOP ----------------
void loop() {
  RTC.getTime(now);
  hh = now.getHour();
  mm = now.getMinutes();
  ss = now.getSeconds();
  unsigned long ms = millis();
  if (ms - lastNTPSync > ntpInterval) {
    lastNTPSync = ms;
    syncTimeFromNTP();
  }
  // Label
  tft.setTextColor(COL_LABEL);
  tft.setTextSize(2);
  tft.setCursor(cx - 48, cy - radius + 65);
  tft.print("RUSSCLOX");
  // Numbers
  tft.setTextColor(YELLOW);
  tft.setTextSize(4);
  tft.setCursor(cx - 19, cy - radius + 26);
  tft.print("12");
  tft.setTextColor(COL_NUMBS);
  tft.setTextSize(3);
  tft.setCursor(cx - radius + 25, cy - 10);
  tft.print("9");
  tft.setCursor(cx + radius - 40, cy - 10);
  tft.print("3");
  tft.setCursor(cx - 10, cy + radius - 45);
  tft.print("6");
  // Digital time under center
  char buf[16];
  // tft.fillRoundRect(cx - 70, cy + 50, 155, 40, 5, BRONZE);
  // tft.fillRoundRect(cx - 65, cy + 55, 145, 30, 5, COL_FACE);
  tft.setTextColor(COL_DIGITAL, COL_FACE);
  tft.setTextSize(3);
  tft.setCursor(cx - 55, cy + 62);
  sprintf(buf, "%02d:%02d", hh, mm);
  tft.print(buf);
  tft.setTextColor(COL_SEC, COL_FACE);
  tft.setTextSize(2);
  sprintf(buf, " %02d", ss);
  tft.print(buf);

  // Inner tick marks
  for (int h = 0; h < 12; h++) {
    float ang = deg2rad(h * 30.0f - 90.0f);
    int16_t x1 = cx + (int16_t)((radius - 175) * cos(ang));
    int16_t y1 = cy + (int16_t)((radius - 175) * sin(ang));
    int16_t x2 = cx + (int16_t)((radius - 160) * cos(ang));
    int16_t y2 = cy + (int16_t)((radius - 160) * sin(ang));
    tft.drawLine(x1, y1, x2, y2, DARKGREY);
  }

  for (int h = 0; h < 6; h++) {
    float ang = deg2rad(h * 60.0f - 90.0f);
    int16_t u1 = cx + (int16_t)((radius - 180) * cos(ang));
    int16_t v1 = cy + (int16_t)((radius - 180) * sin(ang));
    int16_t u2 = cx + (int16_t)((radius - 190) * cos(ang));
    int16_t v2 = cy + (int16_t)((radius - 190) * sin(ang));
    tft.drawLine(u1, v1, u2, v2, LTGREY);
  }

  HandState current = computeHands(hh, mm, ss);
  eraseHands(lastHand);
  drawHands(current);
  lastHand = current;

  // Draw WiFi icon + signal bars
  long rssi = WiFi.RSSI();
  int strength = 0;
  if (rssi > -70) strength = 3;
  else if (rssi > -80) strength = 2;
  else if (rssi > -90) strength = 1;
  drawWiFiIcon(30, 30, strength);
  tft.setTextColor(YELLOW);
  tft.setTextSize(1);
  tft.setCursor(10, 60);
  tft.print("rssi:");
  tft.setTextColor(WHITE, COL_BG);
  tft.print(rssi);
  delay(2);
}
