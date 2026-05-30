//--------------- UPDATED/OPTIMIZED VERSION 5/26/2026
/*┌───────────────────────────────────────────────────────────────────┐
  |                     Animated 2 Dice Roller                        |
  |                          ===========                              |
  |               + Press [SELECT] to roll                            |
  |               + Press [LEFT] to return to title                   |
  |               + Press [RIGHT] to return to title & play EMPIRE!   |
  |               + Connect a buzzer to pin 3 for sound!              |
  |                           ===========                              |
  |               + Re-organised and optimised. 5/26/2026              |
  |───────────────────────────────────────────────────────────────────|
  |                      Russ McEwen 5/13/26                          |
  └───────────────────────────────────────────────────────────────────┘
*/
#include <LiquidCrystal.h>
#define BUZZER_PIN 3
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
#define VERSION 2 

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
#define BPM 140        //  you can change this value changing all the others
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
enum { STATE_INTRO,
       STATE_ROLLING,
       STATE_IDLE,
       STATE_CGRAM };
int gameState = STATE_INTRO;

unsigned long diceAnimTimer = 0;
unsigned long diceRollStart = 0;
bool diceRolling = false;

int diceFinal1 = 0;
int diceFinal2 = 1;
int tempDie1 = 0;
int tempDie2 = 1;

/*──────────────── CGRAM MIRROR ───────────────*/
byte cgram[8][8];
void setCGRAMChar(uint8_t slot, const byte pattern[8]) {
  memcpy(cgram[slot], pattern, 8);
  lcd.createChar(slot, (uint8_t*)pattern);
}

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
void playStartupMelody() {
  beep(880, 120);  // A5
  delay(140);
  beep(1319, 160);  // E6
  delay(180);
  beep(1760, 180);  // A6
  delay(220);
  beep(1568, 120);  // G6
  delay(150);
  beep(1760, 180);  // A6
  delay(220);
}
void playButtonChirp() {
  tone(BUZZER_PIN, 4500, 10);
}
void playChangeMode() {
  beep(4400, 120);
  delay(120);
  beep(3800, 25);
  delay(60);
  beep(3200, 25);
}

void empireDiddy() {
  // the loop routine runs over and over again forever:
  //tone(pin, note, duration)
  tone(3, Ab3, S);  // --- Start in proper scale ---
  delay(1 + S);
  //  --- Begin the tune! ---
  delay(Q);
  tone(3, LA3, Q);
  delay(1 + Q);
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

/*┌───────────────────────────────────────────────────────────────────┐
  |    =================== PIP ENGINE ========================        |
  └───────────────────────────────────────────────────────────────────┘*/
bool pipTL, pipTR, pipBL, pipBR, pipSingle, pipSixL, pipSixR;
byte tile[8];

const byte explosion1[8] PROGMEM = { B00100, B01010, B10101, B01010, B00100, 0, 0, 0 };
const byte explosion2[8] PROGMEM = { B01010, B10101, B01010, B10101, B01010, B10101, B01010, 0 };
const byte explosion3[8] PROGMEM = { 0, 0, B01010, B10101, B01010, 0, 0, 0 };
const byte diamond[8] PROGMEM = { B00100, B01110, B11111, B11111, B11111, B01110, B00100, 0 };
const byte diamond2[8] PROGMEM = { B10101, B01010, B11011, B10001, B11011, B01010, B10101, 0 };

void loadExplosionFrames() {
  byte buf[8];
  memcpy_P(buf, explosion1, 8);
  setCGRAMChar(0, buf);
  memcpy_P(buf, explosion2, 8);
  setCGRAMChar(1, buf);
  memcpy_P(buf, explosion3, 8);
  setCGRAMChar(2, buf);
  memcpy_P(buf, diamond, 8);
  setCGRAMChar(3, buf);
  memcpy_P(buf, diamond2, 8);
  setCGRAMChar(4, buf);
}

void playExplosionBothDice() {
  loadExplosionFrames();
  for (int frame = 0; frame < 3; frame++) {
    tone(BUZZER_PIN, 20 - (frame * 2), 500);
    for (int dy = 0; dy < 2; dy++)
      for (int dx = 0; dx < 2; dx++) {
        lcd.setCursor(6 + dx, dy);
        lcd.write(byte(frame));
        lcd.setCursor(9 + dx, dy);
        lcd.write(byte(frame));
      }
    delay(200);
  }

  // clear full 2×2 areas for both dice
  lcd.setCursor(6, 0);
  lcd.print(F("  "));
  lcd.setCursor(6, 1);
  lcd.print(F("  "));
  lcd.setCursor(9, 0);
  lcd.print(F("  "));
  lcd.setCursor(9, 1);
  lcd.print(F("  "));
}

void setPipsForFace(int face) {
  pipTL = pipTR = pipBL = pipBR = pipSingle = pipSixL = pipSixR = false;
  switch (face) {
    case 1: pipSingle = true; break;
    case 2: pipTL = pipBR = true; break;
    case 3: pipTL = pipSingle = pipBR = true; break;
    case 4: pipTL = pipTR = pipBL = pipBR = true; break;
    case 5: pipTL = pipTR = pipSingle = pipBL = pipBR = true; break;
    case 6: pipTL = pipTR = pipBL = pipBR = pipSixL = pipSixR = true; break;
  }
}

void buildTile(bool left, bool top, int slot) {
  for (int r = 0; r < 8; r++) tile[r] = B00000;
  tile[top ? 0 : 6] = B01111;

  for (int r = 0; r < 8; r++)
    tile[r] |= left ? B01000 : B00001;

  auto putPip = [&](bool pip, int row, int col) {
    if (pip) tile[row] |= (1 << (4 - col));
  };

  if (pipTL && left && top) putPip(true, 2, 3);
  if (pipTR && !left && top) putPip(true, 2, 2);
  if (pipBL && left && !top) putPip(true, 4, 3);
  if (pipBR && !left && !top) putPip(true, 4, 2);
  if (pipSingle && !left && top) putPip(true, 7, 0);
  if (pipSixL && left && top) putPip(true, 7, 3);
  if (pipSixR && !left && top) putPip(true, 7, 2);

  if (!top) tile[7] = B00000;
  if (top && !left) tile[0] = B11111;
  if (!top && !left) tile[6] = B11111;

  lcd.createChar(slot, tile);
}

void loadMegaDie(int dieIndex, int face) {
  setPipsForFace(face);
  int base = dieIndex * 4;
  buildTile(true, true, base + 0);
  buildTile(false, true, base + 1);
  buildTile(true, false, base + 2);
  buildTile(false, false, base + 3);
}

void showEvent(const __FlashStringHelper* a, const __FlashStringHelper* b) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(a);
  lcd.setCursor(0, 1);
  lcd.print(b);
  playVictory();
  delay(500);

  // reset so reward does not repeat
  tempDie1 = 0;
  tempDie2 = 1;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("DICE"));
  lcd.setCursor(0, 1);
  lcd.print(F("GAME"));
}


void drawMegaDice(int f1, int f2) {
  loadMegaDie(0, f1);
  loadMegaDie(1, f2);

  if (tempDie1 == 6 && tempDie2 == 6)
    showEvent(F("    BOXCARS"), F("    BOXCARS"));

  else if (tempDie1 == 1 && tempDie2 == 1)
    showEvent(F("  SNAKE"), F("    EYES!!"));

  else if ((tempDie1 == 1 && tempDie2 == 2) || (tempDie1 == 2 && tempDie2 == 1))
    showEvent(F("  ACEY"), F("   DUECY!!"));

  else if (tempDie1 == tempDie2)
    showEvent(F("  DOUBLE"), F("    TROUBLE"));

  lcd.setCursor(6, 0);
  lcd.write(byte(0));
  lcd.write(byte(1));
  lcd.setCursor(6, 1);
  lcd.write(byte(2));
  lcd.write(byte(3));
  lcd.setCursor(9, 0);
  lcd.write(byte(4));
  lcd.write(byte(5));
  lcd.setCursor(9, 1);
  lcd.write(byte(6));
  lcd.write(byte(7));

  lcd.setCursor(12, 0);
  lcd.print(f1);
  lcd.print(F("|"));
  lcd.print(f2);
}

/*┌───────────────────────────────────────────────────────────────────┐
  |      =================== ROLL ENGINE =======================      |
  └───────────────────────────────────────────────────────────────────┘*/
void startMegaRoll() {
  diceRolling = true;
  diceRollStart = millis();
  gameState = STATE_ROLLING;
  playChangeMode();
  lcd.clear();
  // prevent old reward conditions from triggering during the new roll
  lcd.setCursor(0, 0);
  lcd.print(F("DICE"));
  lcd.setCursor(0, 1);
  lcd.print(F("GAME"));
  tempDie1 = 0;
  tempDie2 = 1;
}

void updateMegaRoll() {
  if (!diceRolling) return;

  if (millis() - diceAnimTimer > 100) {
    diceAnimTimer = millis();
    drawMegaDice(random(1, 7), random(1, 7));
  }

  if (millis() - diceRollStart > random(600, 2000)) {
    diceRolling = false;
    playExplosionBothDice();
    diceFinal1 = random(1, 7);
    diceFinal2 = random(1, 7);
    tempDie1 = diceFinal1;
    tempDie2 = diceFinal2;
    drawMegaDice(diceFinal1, diceFinal2);
    gameState = STATE_IDLE;
  }
}

/*┌───────────────────────────────────────────────────────────────────┐
  |     =================== INTRO SCREEN ======================       |
  └───────────────────────────────────────────────────────────────────┘*/
void showIntro() {
  loadExplosionFrames();
  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 0);
  lcd.print(F("DICE"));
  lcd.setCursor(0, 1);
  lcd.print(F("GAME"));

  if ((millis() / 700) % 2 == 0) {
    lcd.setCursor(6, 0);
    lcd.write(byte(4));
    lcd.print(F("Press "));
    lcd.write(byte(4));
    lcd.setCursor(6, 1);
    lcd.write(byte(3));
    lcd.print(F("      "));
    lcd.write(byte(3));
  } else {
    lcd.setCursor(6, 0);
    lcd.write(byte(3));
    lcd.print(F("      "));
    lcd.write(byte(3));
    lcd.setCursor(6, 1);
    lcd.write(byte(4));
    lcd.print(F("SELECT"));
    lcd.write(byte(4));
  }
}

/*┌───────────────────────────────────────────────────────────────────┐
  |    ===================== BOOT SEQUENCE =======================    |
  └───────────────────────────────────────────────────────────────────┘*/
void bootSeq() {
  for (int i = 22; i > 1; i--) {
    lcd.setCursor(i, 0);
    lcd.write(byte(4));
    lcd.write(byte(3));
    lcd.print(F("  DICE  "));
    lcd.write(byte(3));
    lcd.write(byte(4));
    lcd.print(F(" "));
    delay(80);
    tone(BUZZER_PIN, 1000 / i, 30);
  }
  for (int i = 22; i > 0; i--) {
    lcd.setCursor(i, 1);
    lcd.write(byte(4));
    lcd.write(byte(3));
    lcd.print(F(" by Russ  "));
    lcd.write(byte(3));
    lcd.write(byte(4));
    lcd.print(F(" "));
    delay(80);
    tone(BUZZER_PIN, 400 * i, 50);
  }
  delay(1000);
  for (int i = 22; i >= 0; i--) {
    lcd.setCursor(i, 0);
    lcd.write(byte(4));
    lcd.setCursor(i, 1);
    lcd.write(byte(4));
    // lcd.write(byte(3));
    lcd.print(F(" v"));
    lcd.print(VERSION);
    lcd.print(F(" "));
    // lcd.write(byte(3));
    lcd.write(byte(4));
    delay(40);
    tone(BUZZER_PIN, 400 * i, 50);
  }
  playVictory();
  delay(1000);
  lcd.clear();
}

/*┌───────────────────────────────────────────────────────────────────┐
  |    ======================= SETUP =========================        |
  └───────────────────────────────────────────────────────────────────┘*/
void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  lcd.begin(16, 2);
  randomSeed(analogRead(A5));
  loadExplosionFrames();
  bootSeq();
}

/*┌───────────────────────────────────────────────────────────────────┐
  |    ======================== LOOP =========================        |
  └───────────────────────────────────────────────────────────────────┘*/
void loop() {
  int btn = readButtons();
  updateMegaRoll();

  switch (gameState) {
    case STATE_INTRO:
      showIntro();
      if (btn == 5) startMegaRoll();
      break;

    case STATE_IDLE:
      // lcd.setCursor(0, 0);
      // lcd.print(F("                "));
      // lcd.setCursor(0, 1);
      // lcd.print(F("                "));
      drawMegaDice(diceFinal1, diceFinal2);
      if (btn == 5) startMegaRoll();
      if (btn == 4) gameState = STATE_INTRO;
      if (btn == 1) {
        beep(4400, 120);
        delay(150);
        beep(3800, 25);
        delay(60);
        beep(3200, 25);
        // lcd.clear();
        empireDiddy();
        // gameState = STATE_CGRAM;
      }
      break;

    case STATE_ROLLING:
      break;

    case STATE_CGRAM:
      // CGRAM inspector removed for size
      break;
  }
}
