/*        ------------- FINAL DRAFT -------------
  ============================================================
    Written specifically for UNO R4 w/ built-in 12x8 LED natrix
   UNO R4 — 27‑Waveform Generator with Continuous Scrolling Labels
   Engineering‑Manual Final Draft (Loop‑Driven DAC Engine)
                 😎 -- Russ McEwen 5/1/2026 ---  😎
   ============================================================
   Features:
     • 27 selectable waveforms
     • DAC output on A0 (12‑bit, loop‑timed sample updates)
     • LED Matrix smooth right‑to‑left scrolling labels
     • Auto mode (5‑second waveform cycling)
     • Manual mode via button
         - Short press: next waveform
         - Long press: exit manual mode
     • Universal duty shaping (ON/OFF toggle)
     • A3/A4 duty adjust + fast adjust
         - Single/hold: fine and fast duty % control
     • Double‑tap A4 → duty ON/OFF
         - Immediate waveform rebuild on ON/OFF
         - DUTY ON/OFF + Duty% temporary scroll messages
     • Persistent LED‑matrix indicators
         - Manual‑mode block in upper‑right corner
         - Duty‑enabled status block on matrix

   =======================================================================
   UNO R4 WAVEFORM GENERATOR — WIRING + DISPLAY INDICATORS (ASCII MAP)
   =======================================================================

   ┌───────────────────────────────────────────────────────────────────┐
   │                           WIRING MAP                              │
   └───────────────────────────────────────────────────────────────────┘

      ┌─────────────── UNO R4 (Top View) ─────────────────┐
      │                                                   │
      │   [A0] → DAC OUT  ───────────────► Scope / Output │
      │                                                   │
      │   [A3] → DUTY DOWN  Button  (GND on press)        │
      │   [A4] → DUTY UP    Button  (GND on press)        │
      │   [A5] → MODE Button (Short = Next Waveform)      │
      │                     (Long  = Exit Manual Mode)    │
      │                                                   │
      │   All buttons wired:  One side → Pin              │
      │                           Other side → GND        │
      │                                                   │
      │   LED Matrix → Onboard (no wiring needed)         │
      └───────────────────────────────────────────────────┘


   ┌───────────────────────────────────────────────────────────────────┐
   │                     LED MATRIX INDICATOR LEGEND                  │
   └───────────────────────────────────────────────────────────────────┘

      Scrolling Text:
        • Shows waveform name continuously
        • Temporarily replaced by DUTY messages (Duty %, ON/OFF)

      Manual‑Mode Indicator (top‑right corner):
        ■■
        ■■   ← 2×2 block in upper‑right
        Means: Manual mode ACTIVE (Manual mode button pressed at least once)
                HOLD Manual mode button 1sec to return to Auto mode

      Duty‑Enabled Indicator:
        ■■
        ■■   ← 2×2 block offset left from the corner
        Means: Duty shaping is ON (double‑tap A4/Duty +)
              Double‑tap A3/Duty - to exit duty mode

      Behavior Summary:
        • Duty OFF → Waves are pure, no shaping applied
        • Duty ON  → All waves rebuilt live as duty changes
        • Square wave follows same rules (no secret PWM anymore!)

   ┌───────────────────────────────────────────────────────────────────┐
   │                         BUTTON BEHAVIOR                           │
   └───────────────────────────────────────────────────────────────────┘

      A5 (Mode Button):
        • Short press → Enter manual mode, Next waveform
        • Long press  → Exit manual mode (return to auto cycle)

      A4 (Duty UP):
        • Single tap / hold → Increase duty
        • Double‑tap        → Toggle duty ON/OFF

      A3 (Duty DOWN):
        • Single tap / hold → Decrease duty

      Fast Adjust:
        • Hold EITHER A3 + A4 → Fast duty step
          Duty% will appear when + or - duty buttonn are pressed
          Duty% value is displayed with waveform name, if active
   =======================================================================
*/
/* ============================================================
   UNO R4 — 27‑Waveform Generator with Continuous Scrolling Labels
   Engineering‑Manual Final Draft (Loop‑Driven DAC Engine)
                  -- Russ McEwen 5/1/2026 ---
   ============================================================
*/

#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"

ArduinoLEDMatrix matrix;

// ============================================================
// SECTION 2 — GLOBAL CONSTANTS & CONFIGURATION
// ============================================================

const int DAC_PIN = A0;
const int TABLE_SIZE = 128;
const uint32_t SAMPLE_PERIOD_US = 20;

const uint32_t AUTO_SWITCH_INTERVAL_MS = 5000; //<-------- 5000 = 5sec
const int WAVEFORM_COUNT = 27;

const int BUTTON_PIN = A5;
const int DUTY_UP_PIN = A4;
const int DUTY_DOWN_PIN = A3;

const uint32_t DEBOUNCE_DELAY_MS = 25;
const uint32_t LONG_PRESS_TIME_MS = 1000;

float duty = 0.50f;
const float DUTY_STEP = 0.01f;
const float DUTY_FAST_STEP = 0.05f;

uint32_t lastDutyRepeat = 0;
const uint32_t DUTY_REPEAT_MS = 120;

uint32_t labelReturnTime = 0;
bool dutyEnabled = false;
bool showLiveDuty = false;

uint32_t lastA4TapTime = 0;
bool waitingForSecondTap = false;
const uint32_t DOUBLE_TAP_WINDOW_MS = 500;
const uint32_t DOUBLE_TAP_MIN_GAP_MS = 200;

const uint32_t SCROLL_INTERVAL_MS = 40;
const uint32_t INTRO_SCROLL_INTERVAL_MS = 30;

bool introScrollActive = true;
bool introScrollStarted = false;

// ============================================================
// SECTION 3 — GLOBAL STATE VARIABLES
// ============================================================

uint16_t table[TABLE_SIZE];
volatile uint32_t lastSampleTime = 0;
volatile int sampleIndex = 0;

int currentWaveform = -1;
uint32_t lastAutoSwitchTime = 0;

bool manualMode = false;
bool longPressConsumed = false;

static int lastRaw = HIGH;
static int debounced = HIGH;
static uint32_t lastRawChangeTime = 0;
static uint32_t pressStartTime = 0;

String scrollText = "";
int scrollPosition = 0;
uint32_t lastScrollUpdate = 0;

bool suppressWaveLabel = false;

// ============================================================
// SECTION 3.5 — WAVE NAME LOOKUP
// ============================================================

const char* waveName(int id) {
  switch (id) {
    case 0: return "Sine";
    case 1: return "Triangle";
    case 2: return "Saw Tooth";
    case 3: return "Square";
    case 4: return "EKG Wave";
    case 5: return "CrAzY WaVe!!!";
    case 6: return "Stepped";
    case 7: return "Double Pulse";
    case 8: return "Random Noise";
    case 9: return "Half Rect Sine";
    case 10: return "BLOB!!";
    case 11: return "Bent Saw Tooth";
    case 12: return "Shark Fin";
    case 13: return "Hyper Sine";
    case 14: return "Wobble Pulse";
    case 15: return "Ripple Saw";
    case 16: return "Folded Sine";
    case 17: return "SuperSaw";
    case 18: return "SinFold";
    case 19: return "PulseSweep";
    case 20: return "SpiralWave";
    case 21: return "ChaosSpark";
    case 22: return "Formant";
    case 23: return "BitCrush Saw";
    case 24: return "WindowPulse";
    case 25: return "TentMap";
    case 26: return "FM Sweep";
  }
  return "";
}

// ============================================================
// SECTION 4 — SCROLL ENGINE
// ============================================================

void startScroll(const char* txt) {
  scrollText = String(txt) + "   ";
  scrollPosition = matrix.width();
}

void updateScroll(bool introMode) {
  uint32_t now = millis();
  uint32_t interval = introMode ? INTRO_SCROLL_INTERVAL_MS : SCROLL_INTERVAL_MS;

  // When a temporary message expires, restore the normal scrolling label
  if (!introMode && labelReturnTime && now > labelReturnTime) {
    labelReturnTime = 0;

    if (currentWaveform >= 0) {
      if (dutyEnabled) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%s   Duty %d%%",
                 waveName(currentWaveform),
                 int(duty * 100));
        startScroll(buf);
      } else {
        startScroll(waveName(currentWaveform));
      }
    }
  }

  if (now - lastScrollUpdate < interval) return;
  lastScrollUpdate = now;

  matrix.beginDraw();
  matrix.clear();
  matrix.textFont(Font_4x6);
  matrix.stroke(0xFFFFFFFF);

  if (showLiveDuty) {
    char buf[10];
    sprintf(buf, "%d%%", int(duty * 100));
    matrix.beginText(1, 2, 1);
    matrix.print(buf);
    matrix.endText();
  } else {
    matrix.beginText(scrollPosition, 2, 1);
    matrix.print(scrollText.c_str());
    matrix.endText();
  }

  if (manualMode) {
    matrix.point(matrix.width() - 1, 0);
    matrix.point(matrix.width() - 2, 0);
    matrix.point(matrix.width() - 1, 1);
    matrix.point(matrix.width() - 2, 1);
  }

  if (dutyEnabled) {
    matrix.point(matrix.width() - 11, 0);
    matrix.point(matrix.width() - 12, 0);
    matrix.point(matrix.width() - 11, 1);
    matrix.point(matrix.width() - 12, 1);
  }

  matrix.endDraw();

  scrollPosition--;
  int textWidth = scrollText.length() * 4;

  if (scrollPosition < -textWidth) {
    if (introMode) {
      introScrollActive = false;
      return;
    } else {
      scrollPosition = matrix.width();
    }
  }
}
// ============================================================
// SECTION 5 — BUTTON ENGINE
// ============================================================

void updateButton() {
  int raw = digitalRead(BUTTON_PIN);
  uint32_t now = millis();

  if (raw != lastRaw) {
    lastRawChangeTime = now;
    lastRaw = raw;
  }

  if ((now - lastRawChangeTime) > DEBOUNCE_DELAY_MS) {
    if (debounced != raw) {
      debounced = raw;

      if (debounced == LOW) {
        pressStartTime = now;
        longPressConsumed = false;
        manualMode = true;
      } else {
        if (!longPressConsumed && manualMode) {
          currentWaveform = (currentWaveform + 1) % WAVEFORM_COUNT;
          loadWaveform(currentWaveform);
        }
      }
    }
  }

  if (debounced == LOW && !longPressConsumed) {
    if (now - pressStartTime >= LONG_PRESS_TIME_MS) {
      longPressConsumed = true;
      manualMode = false;
    }
  }
}

// ============================================================
// SECTION 6 — WAVEFORM GENERATORS
// ============================================================

void makeSine() {
  for (int i = 0; i < TABLE_SIZE; i++) {
    float p = float(i) / TABLE_SIZE;
    table[i] = (sin(2 * PI * p) * 0.5f + 0.5f) * 4095;
  }
}

void makeTriangle() {
  for (int i = 0; i < TABLE_SIZE; i++) {
    float p = float(i) / TABLE_SIZE;
    float t = (p < 0.5f) ? p * 2 : (1 - (p - 0.5f) * 2);
    table[i] = t * 4095;
  }
}

void makeSaw() {
  for (int i = 0; i < TABLE_SIZE; i++) {
    table[i] = (float(i) / TABLE_SIZE) * 4095;
  }
}

void makeSquareDuty(float d) {
  int threshold = d * TABLE_SIZE;
  for (int i = 0; i < TABLE_SIZE; i++) {
    table[i] = (i < threshold) ? 4095 : 0;
  }
}

void makeEKG() {
  for (int i = 0; i < TABLE_SIZE; i++) {
    float p = float(i) / TABLE_SIZE;
    float v = 0;

    if (p < 0.08f) {
      float x = p / 0.08f;
      v = 0.15f * sinf(x * 3.14159f);
    } else if (p < 0.12f) {
      float x = (p - 0.08f) / 0.04f;
      v = -0.35f * expf(-20.0f * x);
    } else if (p < 0.16f) {
      float x = (p - 0.12f) / 0.04f;
      v = 1.0f * expf(-40.0f * (x - 0.2f) * (x - 0.2f));
    } else if (p < 0.22f) {
      float x = (p - 0.16f) / 0.06f;
      v = -0.25f * expf(-12.0f * x);
    } else if (p < 0.32f) {
      float x = (p - 0.22f) / 0.10f;
      v = 0.10f + 0.05f * x;
    } else if (p < 0.60f) {
      float x = (p - 0.32f) / 0.28f;
      v = 0.35f * sinf(x * 3.14159f);
    } else {
      v = 0.0f;
    }

    table[i] = (uint16_t)((v + 1.0f) * 2047.0f);
  }
}

void makeCrazy1() {
  for (int i = 0; i < TABLE_SIZE; i++) {
    float p = float(i) / TABLE_SIZE;
    float base = sin(2 * PI * p * 3);
    float warp = sin(2 * PI * p * 17) * 0.3f;
    float noise = (rand() % 1000) / 1000.0f * 0.2f;
    float v = (base + warp + noise) * 0.5f + 0.5f;
    table[i] = constrain(v * 4095, 0, 4095);
  }
}

void makeSteps() {
  for (int i = 0; i < TABLE_SIZE; i++) {
    int step = (i / (TABLE_SIZE / 8));
    table[i] = (step / 7.0f) * 4095;
  }
}

void makeDoublePulse() {
  for (int i = 0; i < TABLE_SIZE; i++) {
    float p = float(i) / TABLE_SIZE;
    float v = expf(-pow((p - 0.25f) * 20, 2)) + expf(-pow((p - 0.75f) * 20, 2));
    v = min(v, 1.0f);
    table[i] = v * 4095;
  }
}

void makeNoise() {
  for (int i = 0; i < TABLE_SIZE; i++) {
    table[i] = rand() % 4096;
  }
}

void makeHalfRect() {
  for (int i = 0; i < TABLE_SIZE; i++) {
    float p = float(i) / TABLE_SIZE;
    float s = sin(2 * PI * p);
    table[i] = (s > 0 ? s : 0) * 4095;
  }
}

void makeBlob() {
  for (int i = 0; i < TABLE_SIZE; i++) {
    float p = float(i) / TABLE_SIZE;
    float v = sin(2 * PI * p) * sin(2 * PI * p * 3);
    table[i] = (v * 0.5f + 0.5f) * 4095;
  }
}

void makeBentSaw() {
  for (int i = 0; i < TABLE_SIZE; i++) {
    float p = float(i) / TABLE_SIZE;
    float v = pow(p, 0.3f);
    table[i] = v * 4095;
  }
}

void makeSharkfin() {
  for (int i = 0; i < TABLE_SIZE; i++) {
    float p = float(i) / TABLE_SIZE;
    float v = pow(p, 0.2f);
    table[i] = v * 4095;
  }
}

void makeHyperSine() {
  for (int i = 0; i < TABLE_SIZE; i++) {
    float p = float(i) / TABLE_SIZE;
    float fm = sin(2 * PI * p * 7) * 0.25f;
    float v = (sin(2 * PI * (p + fm)) * 0.5f + 0.5f);
    table[i] = v * 4095;
  }
}

void makeWobblePulse() {
  for (int i = 0; i < TABLE_SIZE; i++) {
    float p = float(i) / TABLE_SIZE;
    float width = 0.3f + 0.1f * sin(2 * PI * p * 3);
    float v = (p < width) ? 1.0f : 0.0f;
    table[i] = v * 4095;
  }
}

void makeRippleSaw() {
  for (int i = 0; i < TABLE_SIZE; i++) {
    float p = float(i) / TABLE_SIZE;
    float base = p;
    float ripple = sin(2 * PI * p * 20) * 0.1f;
    float v = base + ripple;
    table[i] = constrain(v * 4095, 0, 4095);
  }
}

void makeFoldedSine() {
  for (int i = 0; i < TABLE_SIZE; i++) {
    float p = float(i) / TABLE_SIZE;
    float v = fabs(sin(2 * PI * p));
    table[i] = v * 4095;
  }
}

void makeSuperSaw() {
  for (int i = 0; i < TABLE_SIZE; i++) {
    float p = float(i) / TABLE_SIZE;
    float v = 0;

    for (int k = -3; k <= 3; k++) {
      float detune = 1.0f + k * 0.015f;
      float phase = fmod(p * detune, 1.0f);
      v += phase;
    }

    v /= 7.0f;
    table[i] = v * 4095;
  }
}

void makeSinFold() {
  for (int i = 0; i < TABLE_SIZE; i++) {
    float p = float(i) / TABLE_SIZE;
    float s = sin(2 * PI * p);

    float fold = 2.5f;
    float v = fabs(fmod(s * fold + 1.0f, 2.0f) - 1.0f);

    table[i] = v * 4095;
  }
}

void makePulseSweep() {
  for (int i = 0; i < TABLE_SIZE; i++) {
    float p = float(i) / TABLE_SIZE;
    float width = 0.1f + 0.8f * p;
    float v = (p < width) ? 1.0f : 0.0f;
    table[i] = v * 4095;
  }
}

void makeSpiralWave() {
  for (int i = 0; i < TABLE_SIZE; i++) {
    float p = float(i) / TABLE_SIZE;

    float carrier = sin(2 * PI * p);
    float warp = expf(p * 3.0f);

    float v = (carrier * warp);
    v = (v * 0.5f + 0.5f);

    table[i] = constrain(v * 4095, 0, 4095);
  }
}

void makeChaosSpark() {
  float x = 0.1f;
  float y = 0.0f;

  for (int i = 0; i < TABLE_SIZE; i++) {
    float xn = sin(3.7f * y + 0.01f);
    float yn = sin(3.7f * x + 0.01f);

    x = xn;
    y = yn;

    float v = (x + 1.0f) * 0.5f;
    table[i] = v * 4095;
  }
}

void makeFormant() {
  for (int i = 0; i < TABLE_SIZE; i++) {
    float p = float(i) / TABLE_SIZE;

    float f1 = sin(2 * PI * p * 1.0f);
    float f2 = sin(2 * PI * p * 3.5f);
    float f3 = sin(2 * PI * p * 7.5f);

    float v = (f1 * 0.5f + f2 * 0.3f + f3 * 0.2f) * 0.5f + 0.5f;
    table[i] = constrain(v * 4095, 0, 4095);
  }
}

void makeBitCrushSaw() {
  for (int i = 0; i < TABLE_SIZE; i++) {
    float p = float(i) / TABLE_SIZE;
    float v = p;

    int crushed = int(v * 15.0f);
    v = crushed / 15.0f;

    table[i] = v * 4095;
  }
}

void makeWindowPulse() {
  for (int i = 0; i < TABLE_SIZE; i++) {
    float p = float(i) / TABLE_SIZE;

    float window = 0.5f - 0.5f * cos(2 * PI * p);
    float width = 0.3f + 0.2f * window;

    float v = (p < width) ? 1.0f : 0.0f;
    table[i] = v * 4095;
  }
}

void makeTentMap() {
  float x = 0.3f;

  for (int i = 0; i < TABLE_SIZE; i++) {
    if (x < 0.5f)
      x = x * 2.0f;
    else
      x = 2.0f * (1.0f - x);

    table[i] = x * 4095;
  }
}

void makeFMSweep() {
  for (int i = 0; i < TABLE_SIZE; i++) {
    float p = float(i) / TABLE_SIZE;

    float sweep = sin(2 * PI * p * 2.0f) * 0.4f;
    float v = sin(2 * PI * (p * (1.0f + sweep)));

    table[i] = (v * 0.5f + 0.5f) * 4095;
  }
}
// ============================================================
// SECTION 7 — DUTY BUTTON ENGINE
// ============================================================

void updateDutyButtons() {
  uint32_t now = millis();

  bool up = (digitalRead(DUTY_UP_PIN) == LOW);
  bool down = (digitalRead(DUTY_DOWN_PIN) == LOW);

  // When buttons released → stop live display and trigger scroll message
  if (!up && !down && showLiveDuty) {
    showLiveDuty = false;

    char buf[20];
    sprintf(buf, "Duty %d%%", int(duty * 100));
    startScroll(buf);
    labelReturnTime = millis() + 3000;
  }

  if (!up && !down) {
    showLiveDuty = false;
  }

  // ============================================================
  // A4 DOUBLE‑TAP DETECTOR (DUTY TOGGLE)
  // ============================================================
  static bool a4WasDown = false;
  static uint32_t a4PressTime = 0;

  if (up && !a4WasDown) {
    a4WasDown = true;
    a4PressTime = now;
  }

  if (!up && a4WasDown) {
    uint32_t pressLen = now - a4PressTime;
    a4WasDown = false;

    if (pressLen < 150) {
      if (!waitingForSecondTap) {
        waitingForSecondTap = true;
        lastA4TapTime = now;
      } else {
        uint32_t gap = now - lastA4TapTime;
        if (gap >= DOUBLE_TAP_MIN_GAP_MS && gap <= DOUBLE_TAP_WINDOW_MS) {

          dutyEnabled = !dutyEnabled;

          if (dutyEnabled) {
            startScroll("DUTY ON");
            labelReturnTime = millis() + 5000;
            suppressWaveLabel = true;
            loadWaveform(currentWaveform);
          } else {
            startScroll("DUTY OFF");
            labelReturnTime = millis() + 5000;
            suppressWaveLabel = true;
            loadWaveform(currentWaveform);
          }

          waitingForSecondTap = false;
          return;
        }
      }
    }
  }

  if (waitingForSecondTap && (now - lastA4TapTime > DOUBLE_TAP_WINDOW_MS)) {
    waitingForSecondTap = false;
  }

  // ============================================================
  // DUTY ADJUST (A3/A4 TAP OR HOLD)
  // ============================================================

  if (!up && !down) {
    lastDutyRepeat = now;
    return;
  }

  if (now - lastDutyRepeat >= DUTY_REPEAT_MS) {
    lastDutyRepeat = now;

    if (up) duty += (up && down) ? DUTY_FAST_STEP : DUTY_STEP;
    if (down) duty -= (up && down) ? DUTY_FAST_STEP : DUTY_STEP;

    duty = constrain(duty, 0.0f, 1.0f);

    if (dutyEnabled) {
      loadWaveform(currentWaveform);
    }

    showLiveDuty = true;
  }
}

// ============================================================
// SECTION 8 — DUTY POST‑PROCESSOR
// ============================================================

void applyDutyToTable(float d) {
  int threshold = d * TABLE_SIZE;
  for (int i = 0; i < TABLE_SIZE; i++) {
    if (i >= threshold) {
      table[i] = 0;
    }
  }
}

// ============================================================
// SECTION 9 — WAVEFORM LOADER
// ============================================================

void loadWaveform(int id) {
  currentWaveform = id;

  switch (id) {
    case 0: makeSine(); break;
    case 1: makeTriangle(); break;
    case 2: makeSaw(); break;

    case 3:
      if (dutyEnabled)
        makeSquareDuty(duty);
      else
        makeSquareDuty(0.5f);
      break;

    case 4: makeEKG(); break;
    case 5: makeCrazy1(); break;
    case 6: makeSteps(); break;
    case 7: makeDoublePulse(); break;
    case 8: makeNoise(); break;
    case 9: makeHalfRect(); break;
    case 10: makeBlob(); break;
    case 11: makeBentSaw(); break;
    case 12: makeSharkfin(); break;
    case 13: makeHyperSine(); break;
    case 14: makeWobblePulse(); break;
    case 15: makeRippleSaw(); break;
    case 16: makeFoldedSine(); break;
    case 17: makeSuperSaw(); break;
    case 18: makeSinFold(); break;
    case 19: makePulseSweep(); break;
    case 20: makeSpiralWave(); break;
    case 21: makeChaosSpark(); break;
    case 22: makeFormant(); break;
    case 23: makeBitCrushSaw(); break;
    case 24: makeWindowPulse(); break;
    case 25: makeTentMap(); break;
    case 26: makeFMSweep(); break;
  }

  if (dutyEnabled) {
    applyDutyToTable(duty);
  }

  if (!suppressWaveLabel) {
    if (dutyEnabled) {
      char buf[32];
      snprintf(buf, sizeof(buf), "%s   Duty %d%%",
               waveName(id),
               int(duty * 100));
      startScroll(buf);
    } else {
      startScroll(waveName(id));
    }
  }

  suppressWaveLabel = false;
}
// ============================================================
// SECTION 10 — SETUP
// ============================================================

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(DUTY_UP_PIN, INPUT_PULLUP);
  pinMode(DUTY_DOWN_PIN, INPUT_PULLUP);

  analogWriteResolution(12);
  srand(analogRead(A1));

  matrix.begin();
  matrix.textFont(Font_4x6);

  // Intro scroll starts in loop()
  lastSampleTime = micros();
  lastAutoSwitchTime = millis();
}

// ============================================================
// SECTION 11 — MAIN LOOP
// ============================================================

void loop() {
  uint32_t nowUs = micros();
  uint32_t nowMs = millis();

  // INTRO PHASE — one‑shot slow scroll
  if (introScrollActive) {
    if (!introScrollStarted) {
      startScroll("27 Waveform Generator for UNO-R4-WIFI");
      scrollPosition = matrix.width();
      introScrollStarted = true;
    }

    updateScroll(true);  // slow intro scroll

    // Did intro finish THIS frame?
    if (!introScrollActive) {
      loadWaveform(0);             // start Wave 0 immediately
      lastSampleTime = nowUs;      // start DAC timing
      lastAutoSwitchTime = nowMs;  // start auto timer
    } else {
      return;  // still intro → DAC OFF
    }
  }

  // DAC sample output
  if (nowUs - lastSampleTime >= SAMPLE_PERIOD_US) {
    lastSampleTime += SAMPLE_PERIOD_US;
    analogWrite(DAC_PIN, table[sampleIndex]);
    sampleIndex = (sampleIndex + 1) % TABLE_SIZE;
  }

  // UI
  updateScroll(false);
  updateButton();
  updateDutyButtons();

  // Auto waveform switching
  if (!manualMode) {
    if (nowMs - lastAutoSwitchTime >= AUTO_SWITCH_INTERVAL_MS) {
      lastAutoSwitchTime += AUTO_SWITCH_INTERVAL_MS;
      loadWaveform((currentWaveform + 1) % WAVEFORM_COUNT);
    }
  }
}
