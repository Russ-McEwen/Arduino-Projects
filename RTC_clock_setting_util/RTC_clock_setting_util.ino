/*
   ┌───────────────────────────────────────────────┐
   |   RTC Real Time Clock time setting utility    |
   |===============================================|
   |  This is a utility for quickly setting your   |
   | RTC real time clock module.                   |
   |                💠💠💠💠                     |
   |   🔹There is an adjustment to accommodate     |
   | an exact setting adjustment, to accouont for  |
   | upload time. This will allow you to easily    |
   | adjust your RTC module within a second of     |
   | true internet time.                           |
   |===============================================|
   |   😎 ──── Russ MCewen - 5/6/2026 ──── 😎     |
   └───────────────────────────────────────────────┘
   ┌──────────────────────────────────────────────────────────────┐
   |                     DS3231 RTC WIRING                        |
   |==============================================================|
   |   DS3231 Pin        →        Arduino Pin                     |
   |──────────────────────────────────────────────────────────────|
   |      VCC            →            5V                          |
   |      GND            →            GND                         |
   |      SDA            →            A4   (I²C Data)             |
   |      SCL            →            A5   (I²C Clock)            |
   |──────────────────────────────────────────────────────────────|
   |   Notes:                                                     |
   |    • DS3231 uses I²C — SDA/SCL must be connected correctly   |
   |    • Most breakout boards include pull‑ups already           |
   |    • Install CR2032 battery for timekeeping when powered off |
   |    • Your sketch forces 24‑hour mode (recommended)           |
   |       rtc.setClockMode(false);  // false = 24-hour mode      |
   |==============================================================|
*/
#include <Wire.h>
#include <DS3231.h>
DS3231 rtc;
/*┌────────────────────────────────────────────────────────────────┐
  |====== 🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹 =====|
  |====== 🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹 =====|
  |                                                                |
  | 🟢 Adjust this to compensate for upload delay (in seconds) 🟢 |*/

const int uploadOffset = 7;  //<----Tweak this until it's perfect -😎- |

/*|====== 🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹 =====|
  |====== 🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹🔹 =====|*/
int year, month, day, hour, minute, second;
void parseCompileTime() {
  // --- Parse __DATE__ = "May  6 2026" ---
  char monthStr[4];
  sscanf(__DATE__, "%3s %d %d", monthStr, &day, &year);
  const char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
  month = (strstr(months, monthStr) - months) / 3 + 1;

  // --- Parse __TIME__ = "HH:MM:SS" ---
  sscanf(__TIME__, "%d:%d:%d", &hour, &minute, &second);

  // ---- APPLY OFFSET IN SECONDS ----
  long totalSeconds = hour * 3600L + minute * 60L + second + uploadOffset;

  // --- Track if we rolled past midnight ---
  bool rolledDay = false;
  if (totalSeconds >= 86400L) {
    totalSeconds -= 86400L;
    rolledDay = true;
  } else if (totalSeconds < 0) {
    totalSeconds += 86400L;
    rolledDay = true;
  }

  // Back to h/m/s
  hour = totalSeconds / 3600;
  minute = (totalSeconds % 3600) / 60;
  second = totalSeconds % 60;

  // ---- IF WE CROSSED MIDNIGHT, BUMP THE DATE BY ONE DAY ----
  if (rolledDay) {
    static const int monthLengthNorm[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    auto isLeap = [](int y) {
      return (y % 4 == 0);  // good enough for 2000–2099
    };

    int ml = monthLengthNorm[month - 1];
    if (month == 2 && isLeap(year)) ml++;

    day++;
    if (day > ml) {
      day = 1;
      month++;
      if (month > 12) {
        month = 1;
        year++;
      }
    }
  }
}

void setup() {
  Wire.begin();
  parseCompileTime();
  rtc.setClockMode(false);  // false = 24-hour mode
  rtc.setYear(year - 2000);
  rtc.setMonth(month);
  rtc.setDate(day);
  rtc.setHour(hour);
  rtc.setMinute(minute);
  rtc.setSecond(second);
  Serial.begin(9600);
  for (int i = 0; i < 50; i++) {
    Serial.println("         |");
    // delay(100);
  }
  Serial.println("         V");
  Serial.print("Compile time was: ");
  Serial.println(__TIME__);
  Serial.print("Compiler says today is: ");
  Serial.println(__DATE__);
  Serial.println("RTC set using compile time + offset.");
  Serial.print("Offset was set to ");
  Serial.print(uploadOffset);
  Serial.println(" secs.");
  Serial.print("Time was set to ");
  Serial.print(hour), Serial.print(":"), Serial.print(minute), Serial.print(":"), Serial.println(second);
  Serial.print("Date was set to ");
  Serial.print(month), Serial.print("/"), Serial.print(day), Serial.print("/"), Serial.println(year);
}
void loop() {}
