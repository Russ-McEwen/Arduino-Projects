/*┌───────────────────────────────────────────────────────────────────┐
  |                      -= Photon Dodge =-                           |
  |                          ===========                              |
  |            + Press [SELECT] to start                              |
  |            + Press [UP]/[DOWN] to avoid the  photons              |
  |            + Press [LEFT] to return to Title screen               |
  |            + From title screen, press [UP] to reset high score.   |
  |            + Connect a buzzer to pin 3 for sound!                 |
  |                          ===========                              |
  |───────────────────────────────────────────────────────────────────|
  |                      Russ McEwen 5/13/26                          |
  └───────────────────────────────────────────────────────────────────┘
*/
#include <LiquidCrystal.h>
#include <EEPROM.h>

#define VERSION 2

#define BUZZER_PIN 3
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// NB: ALL NOTES DEFINED WITH STANDARD ENGLISH NAMES, EXCEPT FROM "A"
//THAT IS CALLED WITH THE ITALIAN NAME "LA" BECAUSE A0,A1...ARE THE ANALOG PINS ON ARDUINO.
// (Ab IS CALLED Ab AND NOT LAb)
#define C0 16.35
#define Db0 17.32
#define D0 18.35
#define Eb0 19.45
#define E0 20.60
#define F0 21.83
#define Gb0 23.12
#define G0 24.50
#define Ab0 25.96
#define LA0 27.50
#define Bb0 29.14
#define B0 30.87
#define C1 32.70
#define Db1 34.65
#define D1 36.71
#define Eb1 38.89
#define E1 41.20
#define F1 43.65
#define Gb1 46.25
#define G1 49.00
#define Ab1 51.91
#define LA1 55.00
#define Bb1 58.27
#define B1 61.74
#define C2 65.41
#define Db2 69.30
#define D2 73.42
#define Eb2 77.78
#define E2 82.41
#define F2 87.31
#define Gb2 92.50
#define G2 98.00
#define Ab2 103.83
#define LA2 110.00
#define Bb2 116.54
#define B2 123.47
#define C3 130.81
#define Db3 138.59
#define D3 146.83
#define Eb3 155.56
#define E3 164.81
#define F3 174.61
#define Gb3 185.00
#define G3 196.00
#define Ab3 207.65
#define LA3 220.00
#define Bb3 233.08
#define B3 246.94
#define C4 261.63
#define Db4 277.18
#define D4 293.66
#define Eb4 311.13
#define E4 329.63
#define F4 349.23
#define Gb4 369.99
#define G4 392.00
#define Ab4 415.30
#define LA4 440.00
#define Bb4 466.16
#define B4 493.88
#define C5 523.25
#define Db5 554.37
#define D5 587.33
#define Eb5 622.25
#define E5 659.26
#define F5 698.46
#define Gb5 739.99
#define G5 783.99
#define Ab5 830.61
#define LA5 880.00
#define Bb5 932.33
#define B5 987.77
#define C6 1046.50
#define Db6 1108.73
#define D6 1174.66
#define Eb6 1244.51
#define E6 1318.51
#define F6 1396.91
#define Gb6 1479.98
#define G6 1567.98
#define Ab6 1661.22
#define LA6 1760.00
#define Bb6 1864.66
#define B6 1975.53
#define C7 2093.00
#define Db7 2217.46
#define D7 2349.32
#define Eb7 2489.02
#define E7 2637.02
#define F7 2793.83
#define Gb7 2959.96
#define G7 3135.96
#define Ab7 3322.44
#define LA7 3520.01
#define Bb7 3729.31
#define B7 3951.07
#define C8 4186.01
#define Db8 4434.92
#define D8 4698.64
#define Eb8 4978.03
// DURATION OF THE NOTES
#define BPM 120        //  you can change this value changing all the others
#define H 2 * Q        //half 2/4
#define Q 60000 / BPM  //quarter 1/4
#define E Q / 2        //eighth 1/8
#define S Q / 4        // sixteenth 1/16
#define W 4 * Q        // whole 4/4

/*┌───────────────────────────────────────────────────────────────────┐
  |      ─────────────────── BUTTON READER ────────────────────       |
  └───────────────────────────────────────────────────────────────────┘*/
int readButtons() {
  int x = analogRead(A0);
  if (x < 50) return 1;   // RIGHT
  if (x < 200) return 2;  // UP
  if (x < 400) return 3;  // DOWN
  if (x < 600) return 4;  // LEFT
  if (x < 800) return 5;  // SELECT
  return 0;
}

/*┌───────────────────────────────────────────────────────────────────┐
  |     =================== GAME STATES =======================       |
  └───────────────────────────────────────────────────────────────────┘*/
enum { PD_TITLE,
       PD_GAME,
       PD_SCORE,
       PD_CGRAM };
int pd_state = PD_TITLE;
int btn;
bool pd_waitRelease = false;
int pd_lastBtn = 0;

uint16_t pd_highScore = 0;
uint16_t pd_lastScore = 0;

uint8_t photonFrame = 0;

const int EE_PHOTON_HIGHSCORE = 0;
unsigned long pd_resetHoldStart = 0;
bool pd_resetPending = false;

/*──────────────── CGRAM GLOBALS ───────────────*/
unsigned long cgr_lastRowStep = 0;
unsigned long cgr_lastSlotStep = 0;
static bool cgr_rowHeld = false;
bool cgr_slotHeld = false;
const unsigned long cgr_slotRepeatDelay = 220;  // ms between slot steps
const unsigned long cgr_rowRepeatDelay = 180;   // ms between row steps
int currentBtn;                                 // or whatever you use
int cgr_slot = 0;                               // 0–7
int cgr_row = 0;                                // 0–7
byte cgram[8][8];                               // RAM mirror of CGRAM

/*┌───────────────────────────────────────────────────────────────────┐
  |        ================= MUSICAL SECTION =================        |
  └───────────────────────────────────────────────────────────────────┘*/
inline void beep(uint16_t f, uint16_t d) {
  tone(BUZZER_PIN, f, d);
}

void playVictory() {
  beep(1319, 60);  // E6
  delay(170);
  beep(1319, 60);  // E6
  delay(170);
  beep(1319, 200);  // E6
  delay(200);
  beep(1760, 650);  // A6
  delay(650);
}

void playButtonChirp() {
  tone(BUZZER_PIN, 4500, 10);
}

void empireDiddy() {
  // the loop routine runs over and over again forever:
  //tone(pin, note, duration)
  tone(3, LA3, Q);
  delay(1 + Q);  //delay duration should always be 1 ms more than the note in order to separate them.
  tone(3, LA3, Q);
  delay(1 + Q);
  tone(3, LA3, Q);
  delay(1 + Q);
  tone(3, F3, E + S);
  delay(1 + E + S);
  tone(3, C4, S);
  delay(1 + S);

  tone(3, LA3, Q);
  delay(1 + Q);
  tone(3, F3, E + S);
  delay(1 + E + S);
  tone(3, C4, S);
  delay(1 + S);
  tone(3, LA3, H);
  delay(1 + H);

  tone(3, E4, Q);
  delay(1 + Q);
  tone(3, E4, Q);
  delay(1 + Q);
  tone(3, E4, Q);
  delay(1 + Q);
  tone(3, F4, E + S);
  delay(1 + E + S);
  tone(3, C4, S);
  delay(1 + S);

  tone(3, Ab3, Q);
  delay(1 + Q);
  tone(3, F3, E + S);
  delay(1 + E + S);
  tone(3, C4, S);
  delay(1 + S);
  tone(3, LA3, H);
  delay(1 + H);

  tone(3, LA4, Q);
  delay(1 + Q);
  tone(3, LA3, E + S);
  delay(1 + E + S);
  tone(3, LA3, S);
  delay(1 + S);
  tone(3, LA4, Q);
  delay(1 + Q);
  tone(3, Ab4, E + S);
  delay(1 + E + S);
  tone(3, G4, S);
  delay(1 + S);

  tone(3, Gb4, S);
  delay(1 + S);
  tone(3, E4, S);
  delay(1 + S);
  tone(3, F4, E);
  delay(1 + E);
  delay(1 + E);  //PAUSE
  tone(3, Bb3, E);
  delay(1 + E);
  tone(3, Eb4, Q);
  delay(1 + Q);
  tone(3, D4, E + S);
  delay(1 + E + S);
  tone(3, Db4, S);
  delay(1 + S);

  tone(3, C4, S);
  delay(1 + S);
  tone(3, B3, S);
  delay(1 + S);
  tone(3, C4, E);
  delay(1 + E);
  delay(1 + E);  //PAUSE QUASI FINE RIGA
  tone(3, F3, E);
  delay(1 + E);
  tone(3, Ab3, Q);
  delay(1 + Q);
  tone(3, F3, E + S);
  delay(1 + E + S);
  tone(3, LA3, S);
  delay(1 + S);

  tone(3, C4, Q);
  delay(1 + Q);
  tone(3, LA3, E + S);
  delay(1 + E + S);
  tone(3, C4, S);
  delay(1 + S);
  tone(3, E4, H);
  delay(1 + H);

  tone(3, LA4, Q);
  delay(1 + Q);
  tone(3, LA3, E + S);
  delay(1 + E + S);
  tone(3, LA3, S);
  delay(1 + S);
  tone(3, LA4, Q);
  delay(1 + Q);
  tone(3, Ab4, E + S);
  delay(1 + E + S);
  tone(3, G4, S);
  delay(1 + S);

  tone(3, Gb4, S);
  delay(1 + S);
  tone(3, E4, S);
  delay(1 + S);
  tone(3, F4, E);
  delay(1 + E);
  delay(1 + E);  //PAUSE
  tone(3, Bb3, E);
  delay(1 + E);
  tone(3, Eb4, Q);
  delay(1 + Q);
  tone(3, D4, E + S);
  delay(1 + E + S);
  tone(3, Db4, S);
  delay(1 + S);

  tone(3, C4, S);
  delay(1 + S);
  tone(3, B3, S);
  delay(1 + S);
  tone(3, C4, E);
  delay(1 + E);
  delay(1 + E);  //PAUSE QUASI FINE RIGA
  tone(3, F3, E);
  delay(1 + E);
  tone(3, Ab3, Q);
  delay(1 + Q);
  tone(3, F3, E + S);
  delay(1 + E + S);
  tone(3, C4, S);
  delay(1 + S);

  tone(3, LA3, Q);
  delay(1 + Q);
  tone(3, F3, E + S);
  delay(1 + E + S);
  tone(3, C4, S);
  delay(1 + S);
  tone(3, LA3, H);
  delay(1 + H);

  delay(H);
  return;
}

/*──────────────── SPRITES (PROGMEM) ───────────────*/

const byte Diamond[8] PROGMEM = {
  B00100,
  B01110,
  B11111,
  B11111,
  B11111,
  B01110,
  B00100,
  0
};

const byte pd_shipChar[8] PROGMEM = {
  B01111,
  B01100,
  B11110,
  B11111,
  B11110,
  B01100,
  B01111,
  0
};

const byte pd_photonA[8] PROGMEM = {
  0, B00100, B01110, B11111, B01110, B00100, 0, 0
};
const byte pd_photonB[8] PROGMEM = {
  0, 0, B00100, B01110, B11111, B01110, B00100, 0
};
const byte pd_photonC[8] PROGMEM = {
  0, 0, 0, B00100, B01110, B11111, B01110, B00100
};

const byte pd_explode1[8] PROGMEM = {
  B00100, B01010, B10101, B01010, B00100, 0, 0, 0
};
const byte pd_explode2[8] PROGMEM = {
  B01010, B10101, B01010, B10101, B01010, B10101, B01010, 0
};
const byte pd_explode3[8] PROGMEM = {
  0, 0, B01010, B10101, B01010, 0, 0, 0
};

void setCGRAMChar(uint8_t slot, const byte *src) {
  uint8_t buf[8];
  memcpy_P(buf, src, 8);
  lcd.createChar(slot, buf);
}

// void setCGRAMChar(uint8_t slot, const byte pattern[8]) {
//   memcpy(cgram[slot], pattern, 8);          // save to RAM
//   lcd.createChar(slot, (uint8_t*)pattern);  // write to LCD
// }


void loadCGRAM() {
  setCGRAMChar(0, pd_shipChar);
  setCGRAMChar(7, pd_photonA);
  setCGRAMChar(1, pd_photonA);
  setCGRAMChar(2, pd_photonB);
  setCGRAMChar(3, pd_photonC);
  setCGRAMChar(4, pd_explode1);
  setCGRAMChar(5, pd_explode2);
  setCGRAMChar(6, pd_explode3);
}

/*┌───────────────────────────────────────────────────────────────────┐
  |     =================== INTRO SCREEN ======================       |
  └───────────────────────────────────────────────────────────────────┘*/
void pd_showTitle() {
  lcd.setCursor(0, 0);
  lcd.print(F("PHOTON DODGE v"));
  lcd.print(VERSION);

  lcd.setCursor(0, 1);
  if ((millis() / 1000) % 2 == 0) {
    lcd.print(F("Press SELECT    "));
  } else {
    lcd.print(F("HIGH SCORE: "));
    lcd.print(pd_highScore);
    lcd.print(F("   "));
  }
}

/*┌───────────────────────────────────────────────────────────────────┐
  |    ===================== BOOT SEQUENCE =======================    |
  └───────────────────────────────────────────────────────────────────┘*/
void bootSeq() {
  setCGRAMChar(6, pd_photonA);
  setCGRAMChar(7, Diamond);
  for (int i = 22; i > 0; i--) {
    lcd.setCursor(i, 0);
    lcd.write(byte(7));
    lcd.write(byte(6));
    lcd.print(F("PHOTON"));
    lcd.write(byte(6));
    lcd.write(byte(7));
    lcd.print(F(" "));
    delay(80);
    tone(BUZZER_PIN, 1000 / i, 30);
  }
  for (int i = 22; i > 0; i--) {
    lcd.setCursor(i, 1);
    lcd.write(byte(7));
    lcd.write(byte(6));
    lcd.print(F("DODGE "));
    lcd.write(byte(6));
    lcd.write(byte(7));
    lcd.print(F(" "));
    delay(80);
    tone(BUZZER_PIN, 400 * i, 50);
  }
  delay(1000);
  for (int i = 22; i >= 0; i--) {
    lcd.setCursor(i, 0);
    lcd.write(byte(6));
    lcd.setCursor(i, 1);
    lcd.write(byte(7));
    lcd.write(byte(6));
    lcd.print(F("v"));
    lcd.print(VERSION);
    // lcd.write(byte(3));
    lcd.write(byte(6));
    lcd.write(byte(7));
    lcd.write(byte(6));
    lcd.print(F("RJM"));
    lcd.write(byte(6));
    lcd.write(byte(7));
    lcd.write(byte(6));
    // lcd.print(F(" "));
    delay(20);
    tone(BUZZER_PIN, 400 * i, 50);
  }
  // playVictory();
  delay(3000);
  lcd.clear();
}

/*──────────────── GAME LOOP ───────────────*/
void pd_runGame() {
  int shipRow = 1;
  int photonX = 15;
  int photonRow = random(0, 2);
  int speed = 140;
  int score = 0;

  setCGRAMChar(6, pd_shipChar);
  setCGRAMChar(7, pd_photonA);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("PHOTON DODGE"));
  delay(600);

  while (true) {
    int btn = readButtons();

    if (btn == 2) shipRow = 0;
    if (btn == 3) shipRow = 1;
    if (btn == 5) break;

    photonFrame = (photonFrame + 1) % 3;
    if (photonFrame == 0) setCGRAMChar(7, pd_photonA);
    if (photonFrame == 1) setCGRAMChar(7, pd_photonB);
    if (photonFrame == 2) setCGRAMChar(7, pd_photonC);

    lcd.clear();

    setCGRAMChar(6, pd_shipChar);
    lcd.setCursor(0, shipRow);
    lcd.write(byte(6));

    lcd.setCursor(photonX, photonRow);
    lcd.write(byte(7));

    if (photonX == 0 && photonRow == shipRow) {
      lcd.setCursor(0, shipRow);

      setCGRAMChar(6, pd_explode1);
      lcd.write(byte(6));
      beep(200, 40);
      delay(80);

      setCGRAMChar(6, pd_explode2);
      lcd.setCursor(0, shipRow);
      lcd.write(byte(6));
      beep(180, 40);
      delay(80);

      setCGRAMChar(6, pd_explode3);
      lcd.setCursor(0, shipRow);
      lcd.write(byte(6));
      beep(150, 60);
      delay(120);

      break;
    }

    tone(BUZZER_PIN, ((photonX * speed) + 2000), 20);
    photonX--;

    if (photonX < 0) {
      photonX = 15;
      photonRow = random(0, 2);
      score++;
      beep(1200, 40);
      if (speed > 35) speed -= 3;
    }

    delay(speed);
  }

  pd_lastScore = score;
  pd_state = PD_SCORE;
  pd_waitRelease = true;
}

/*──────────────── SCORE SCREEN ───────────────*/
void pd_showScore() {
  int b = readButtons();

  if (pd_waitRelease) {
    if (b != 5) pd_waitRelease = false;
    pd_lastBtn = b;
    return;
  }

  // if (pd_lastScore == pd_highScore) {
  //   // pd_highScore = pd_lastScore;
  //   lcd.setCursor(0, 0);
  //   lcd.print(F("  *IT'S A TIE!*"));
  //   lcd.setCursor(0, 1);
  //   lcd.print(F(" Score= "));
  //   lcd.print(pd_lastScore);
  //   // delay(1500);
  //   //   EEPROM.write(EE_PHOTON_HIGHSCORE, pd_highScore & 0xFF);
  //   //   EEPROM.write(EE_PHOTON_HIGHSCORE + 1, pd_highScore >> 8);
  //   return;
  // }

  if (pd_lastScore > pd_highScore) {
    pd_highScore = pd_lastScore;
    lcd.setCursor(0, 0);
    lcd.print(F("  *YOU WIN!*"));
    lcd.setCursor(0, 1);
    lcd.print(F(" Score= "));
    lcd.print(pd_lastScore);
    playVictory();
    // delay(1500);
    EEPROM.write(EE_PHOTON_HIGHSCORE, pd_highScore & 0xFF);
    EEPROM.write(EE_PHOTON_HIGHSCORE + 1, pd_highScore >> 8);
    return;
  }

  lcd.setCursor(0, 0);
  lcd.print(F("SCORE: "));
  lcd.print(pd_lastScore);
  lcd.print(F("      "));

  lcd.setCursor(0, 1);
  lcd.print(F("SEL=new L/R=exit"));

  if (b == 5 && pd_lastBtn != 5) {
    pd_state = PD_GAME;
    delay(200);
  }

  if ((b == 1 || b == 4) && pd_lastBtn != b) {
    lcd.clear();
    pd_state = PD_TITLE;
  }

  pd_lastBtn = b;
}

/*┌───────────────────────────────────────────────────────────────────┐
  |    ==================== C-GRAM INSPECTOR ======================   |
  └───────────────────────────────────────────────────────────────────┘*/
void modeCGRAMInspector() {
  // read your button into btn however you normally do
  // loadExplosionFrames();

  loadCGRAM();

  int btn = readButtons();  // or whatever you use
  lcd.setCursor(6, 0);
  lcd.print("CGRAM Insp");

  // ── SLOT navigation (UP/DOWN) with throttle ─────────────────────────────
  if (btn == 4) {  // UP or DOWN
    pd_state = PD_TITLE;
  }

  if (btn == 2 || btn == 3) {  // UP or DOWN
    unsigned long now = millis();

    if (!cgr_slotHeld) {
      // First press = instant
      if (btn == 2) cgr_slot--;
      if (btn == 3) cgr_slot++;

      if (cgr_slot < 0) cgr_slot = 7;
      if (cgr_slot > 7) cgr_slot = 0;
      playButtonChirp();

      cgr_slotHeld = true;
      cgr_lastSlotStep = now;
    } else if (now - cgr_lastSlotStep > cgr_slotRepeatDelay) {
      // Held repeat
      if (btn == 2) cgr_slot--;
      if (btn == 3) cgr_slot++;

      if (cgr_slot < 0) cgr_slot = 7;
      if (cgr_slot > 7) cgr_slot = 0;
      playButtonChirp();

      cgr_lastSlotStep = now;
    }
  } else {
    cgr_slotHeld = false;
  }

  if (btn == 1 || btn == 5) {
    unsigned long now = millis();

    if (!cgr_rowHeld) {
      // first press = instant
      playButtonChirp();
      if (btn == 1) cgr_row++;
      if (btn == 5) cgr_row--;
      if (cgr_row > 7) cgr_row = 0;
      if (cgr_row < 0) cgr_row = 7;
      cgr_rowHeld = true;
      cgr_lastRowStep = now;
    } else if (now - cgr_lastRowStep > cgr_rowRepeatDelay) {
      // held repeat
      playButtonChirp();
      if (btn == 1) cgr_row++;
      if (btn == 5) cgr_row--;
      if (cgr_row > 7) cgr_row = 0;
      if (cgr_row < 0) cgr_row = 7;
      cgr_lastRowStep = now;
    }
  } else {
    cgr_rowHeld = false;
  }


  // ── fetch row pattern from mirror ──────────
  byte row = cgram[cgr_slot][cgr_row];  // 5 LSBs used

  // ── first line: slot/row header ────────────
  lcd.setCursor(0, 0);
  lcd.print(F("S"));
  lcd.print(cgr_slot);
  lcd.print(F(" R"));
  lcd.print(cgr_row);
  // lcd.print(F("      "));  // clear tail

  // ── second line: pixels + hex + binary ─────
  lcd.setCursor(0, 1);

  // pixel view (5 bits → 5 chars)
  for (int b = 4; b >= 0; b--) {
    bool on = row & (1 << b);
    lcd.print(on ? '#' : '.');  // or 0xFF vs ' '
  }

  lcd.print(' ');

  // hex (2 digits)
  if (row < 16) lcd.print('0');
  lcd.print(row, HEX);

  lcd.print(' ');

  // binary (5 bits)
  for (int b = 4; b >= 0; b--) {
    lcd.print((row & (1 << b)) ? '1' : '0');
  }
  // pd_state == STATE_CGRAM;
}

/*┌───────────────────────────────────────────────────────────────────┐
  |    ======================= SETUP =========================        |
  └───────────────────────────────────────────────────────────────────┘*/
void setup() {
  // pinMode(3, OUTPUT);
  // pinMode(3, OUTPUT);
  // digitalWrite(3, LOW);
  pinMode(BUZZER_PIN, OUTPUT);
  lcd.begin(16, 2);
  randomSeed(analogRead(A5));

  uint8_t lo = EEPROM.read(EE_PHOTON_HIGHSCORE);
  uint8_t hi = EEPROM.read(EE_PHOTON_HIGHSCORE + 1);
  pd_highScore = lo | (hi << 8);
  bootSeq();
  lcd.clear();
}

/*┌───────────────────────────────────────────────────────────────────┐
  |    ======================== LOOP =========================        |
  └───────────────────────────────────────────────────────────────────┘*/
void loop() {
  switch (pd_state) {
    case PD_TITLE:
      {
        pd_showTitle();
        int btn = readButtons();

        // --- Start Game ---
        if (btn == 5) {
          playButtonChirp();
          pd_state = PD_GAME;
          delay(200);
          break;
        }

        // --- CGRAM Insp ---
        if (btn == 1) {
          playButtonChirp();
          // pd_state = PD_CGRAM;
          empireDiddy();
          pd_state = PD_TITLE;
          delay(200);
          break;
        }

        if (btn == 3) {
          playButtonChirp();
          lcd.clear();
          bootSeq();
          pd_state = PD_TITLE;
          delay(200);
          break;
        }

        // --- High Score Reset (Hold UP for 2 seconds) ---
        if (btn == 2) {  // UP button
          // lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print(F(" HOLD TO RESET    "));
          lcd.setCursor(0, 1);
          lcd.print(F("   HIGH SCORE     "));
          delay(100);
          if (!pd_resetPending) {
            pd_resetPending = true;
            pd_resetHoldStart = millis();
          } else {
            if (millis() - pd_resetHoldStart > 2000) {
              // Perform reset
              pd_highScore = 0;
              EEPROM.write(EE_PHOTON_HIGHSCORE, 0);
              EEPROM.write(EE_PHOTON_HIGHSCORE + 1, 0);

              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print(F("HIGH SCORE RESET"));
              lcd.setCursor(0, 1);
              lcd.print(F("     COMPLETE"));
              playVictory();
              lcd.clear();
              delay(1200);

              pd_resetPending = false;
            }
          }
        } else {
          pd_resetPending = false;
        }

        break;
      }

    case PD_GAME:
      pd_runGame();
      break;

    case PD_SCORE:
      pd_showScore();
      if (btn == 3) {
        playButtonChirp();
        lcd.clear();
        bootSeq();
        pd_state = PD_TITLE;
        delay(200);
        break;
      }
      break;

    case PD_CGRAM:
      modeCGRAMInspector();
      break;
  }
}
