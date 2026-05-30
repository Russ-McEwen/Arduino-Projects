// ============================================================
// UNO R4 — 12‑Waveform Generator with Continuous Scrolling Labels
// Engineering‑Manual Final Draft (Loop‑Driven DAC Engine)
// ============================================================
//
// Features:
//   • 22 selectable waveforms
//   • DAC output on A0 (12‑bit, loop‑timed sample updates)
//   • LED Matrix smooth right‑to‑left scrolling labels
//   • Auto mode (5‑second waveform cycling)
//   • Manual mode via button
//       - Short presas: next waveform
//       - Long press: exit manual mode
//
// Note on timing:
//   SAMPLE_PERIOD_US is the *target* interval between DAC updates.
//   Actual effective rate is slightly slower due to loop + DAC + UI overhead.
//
// ============================================================
// SECTION 1 — SYSTEM INCLUDES & GLOBAL OBJECTS
// ============================================================

#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"

ArduinoLEDMatrix matrix;


// ============================================================
// SECTION 2 — GLOBAL CONSTANTS & CONFIGURATION
// ============================================================

// ----------------------------
// DAC Configuration
// ----------------------------
const int DAC_PIN = A0;
const int TABLE_SIZE = 128;
// const int TABLE_SIZE = 256;
// Target sample period; real effective period will be a bit longer
const uint32_t SAMPLE_PERIOD_US = 20;  // target ~20 kHz update rate

// ----------------------------
// Auto Waveform Switching
// ----------------------------
const uint32_t AUTO_SWITCH_INTERVAL_MS = 5000;  // 5 seconds
const int WAVEFORM_COUNT = 22;

// ----------------------------
// Button Configuration
// ----------------------------
const int BUTTON_PIN = A5;
const uint32_t DEBOUNCE_DELAY_MS = 25;
const uint32_t LONG_PRESS_TIME_MS = 1000;

// ----------------------------
// Scroll Engine
// ----------------------------
const uint32_t SCROLL_INTERVAL_MS = 50;


// ============================================================
// SECTION 3 — GLOBAL STATE VARIABLES
// ============================================================

// DAC State
uint16_t table[TABLE_SIZE];
volatile uint32_t lastSampleTime = 0;
volatile int sampleIndex = 0;

// Waveform State
int currentWaveform = 0;
uint32_t lastAutoSwitchTime = 0;

// Button State Machine
bool manualMode = false;
bool longPressConsumed = false;

// Button debounce internals
static int lastRaw = HIGH;
static int debounced = HIGH;
static uint32_t lastRawChangeTime = 0;
static uint32_t pressStartTime = 0;

// Scroll Engine State
String scrollText = "Russ MADE!!   ";
int scrollPosition = 0;
uint32_t lastScrollUpdate = 0;


// ============================================================
// SECTION 4 — SCROLL ENGINE
// ============================================================

void startScroll(const char* txt) {
  scrollText = String(txt) + "   ";
  scrollPosition = matrix.width();  // start just off the right edge
}

void updateScroll() {
  uint32_t now = millis();
  if (now - lastScrollUpdate < SCROLL_INTERVAL_MS) return;
  lastScrollUpdate = now;

  matrix.beginDraw();
  matrix.clear();
  matrix.textFont(Font_4x6);
  matrix.stroke(0xFFFFFFFF);

  // Draw scrolling text
  matrix.beginText(scrollPosition, 2, 1);
  matrix.print(scrollText.c_str());
  matrix.endText();

  // Manual mode indicator (4‑pixel corner marker)
  if (manualMode) {
    // Upper‑right 2×2 block
    matrix.point(matrix.width() - 1, 0);  // top‑right
    matrix.point(matrix.width() - 2, 0);  // left of top‑right
    matrix.point(matrix.width() - 1, 1);  // below top‑right
    matrix.point(matrix.width() - 2, 1);  // completes 2×2 block
  }

  matrix.endDraw();

  // Move left
  scrollPosition--;

  int textWidth = scrollText.length() * 4;
  if (scrollPosition < -textWidth)
    scrollPosition = matrix.width();
}


// ============================================================
// SECTION 5 — BUTTON ENGINE (SHORT + LONG PRESS)
// ============================================================
//
// Behavior:
//   • Any press enters manual mode
//   • Short press → next waveform
//   • Long press → exit manual mode
//

void updateButton() {
  int raw = digitalRead(BUTTON_PIN);
  uint32_t now = millis();

  // Detect raw change
  if (raw != lastRaw) {
    lastRawChangeTime = now;
    lastRaw = raw;
  }

  // Debounce: update debounced state only after stable period
  if ((now - lastRawChangeTime) > DEBOUNCE_DELAY_MS) {
    if (debounced != raw) {
      debounced = raw;

      // PRESS EDGE
      if (debounced == LOW) {
        pressStartTime = now;
        longPressConsumed = false;
        manualMode = true;  // entering manual mode
      }

      // RELEASE EDGE
      else {
        if (!longPressConsumed && manualMode) {
          currentWaveform = (currentWaveform + 1) % WAVEFORM_COUNT;
          loadWaveform(currentWaveform);
        }
      }
    }
  }

  // LONG PRESS detection
  if (debounced == LOW && !longPressConsumed) {
    if (now - pressStartTime >= LONG_PRESS_TIME_MS) {
      longPressConsumed = true;
      manualMode = false;  // return to auto mode
    }
  }
}


// ============================================================
// SECTION 6 — WAVEform GENERATORS
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

void makeSquare() {
  for (int i = 0; i < TABLE_SIZE; i++) {
    table[i] = (i < TABLE_SIZE / 2) ? 4095 : 0;
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

// ============================================================
// SECTION 7 — WAVEFORM LOADER
// ============================================================

void loadWaveform(int id) {
  switch (id) {
    case 0:
      makeSine();
      startScroll("Sine");
      break;
    case 1:
      makeTriangle();
      startScroll("Triangle");
      break;
    case 2:
      makeSaw();
      startScroll("Saw Tooth");
      break;
    case 3:
      makeSquare();
      startScroll("Square");
      break;
    case 4:
      makeEKG();
      startScroll("EKG Wave");
      break;
    case 5:
      makeCrazy1();
      startScroll("CRAZY!!!");
      break;
    case 6:
      makeSteps();
      startScroll("Stepped");
      break;
    case 7:
      makeDoublePulse();
      startScroll("Double Pulse");
      break;
    case 8:
      makeNoise();
      startScroll("Random Noise");
      break;
    case 9:
      makeHalfRect();
      startScroll("Half Rect Sine");
      break;
    case 10:
      makeBlob();
      startScroll("BLOB!!");
      break;
    case 11:
      makeBentSaw();
      startScroll("Bent Saw Tooth");
      break;
    case 12:
      makeSharkfin();
      startScroll("Shark Fin");
      break;
    case 13:
      makeHyperSine();
      startScroll("Hyper Sine");
      break;
    case 14:
      makeWobblePulse();
      startScroll("Wobble Pulse");
      break;
    case 15:
      makeRippleSaw();
      startScroll("Ripple Saw");
      break;
    case 16:
      makeFoldedSine();
      startScroll("Folded Sine");
      break;
    case 17:
      makeSuperSaw();
      startScroll("SuperSaw");
      break;
    case 18:
      makeSinFold();
      startScroll("SinFold");
      break;
    case 19:
      makePulseSweep();
      startScroll("PulseSweep");
      break;
    case 20:
      makeSpiralWave();
      startScroll("SpiralWave");
      break;
    case 21:
      makeChaosSpark();
      startScroll("ChaosSpark");
      break;
  }
}


// ============================================================
// SECTION 8 — SETUP
// ============================================================

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  analogWriteResolution(12);
  srand(analogRead(A1));
  matrix.begin();
  matrix.textFont(Font_4x6);
  matrix.stroke(0xFFFFFFFF);
  loadWaveform(0);
  lastSampleTime = micros();
  lastAutoSwitchTime = millis();
}


// ============================================================
// SECTION 9 — MAIN LOOP
// ============================================================

void loop() {
  uint32_t nowUs = micros();
  uint32_t nowMs = millis();

  // DAC sample output (loop‑driven timing)
  if (nowUs - lastSampleTime >= SAMPLE_PERIOD_US) {
    lastSampleTime += SAMPLE_PERIOD_US;
    analogWrite(DAC_PIN, table[sampleIndex]);
    sampleIndex = (sampleIndex + 1) % TABLE_SIZE;
  }

  // UI
  updateScroll();
  updateButton();

  // Auto waveform switching
  if (!manualMode) {
    if (nowMs - lastAutoSwitchTime >= AUTO_SWITCH_INTERVAL_MS) {
      lastAutoSwitchTime += AUTO_SWITCH_INTERVAL_MS;
      currentWaveform = (currentWaveform + 1) % WAVEFORM_COUNT;
      loadWaveform(currentWaveform);
    }
  }
}
