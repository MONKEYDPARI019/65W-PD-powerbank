#include <Wire.h>
#include <Adafruit_INA260.h>
#include <U8g2lib.h>

U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0);
Adafruit_INA260 ina260;

int calcSoC(float voltage) {
  if (voltage >= 12.6) return 100;
  if (voltage <= 9.0)  return 0;
  return (int)((voltage - 9.0) / (12.6 - 9.0) * 100.0);
}

// Battery symbol at (x,y), width=38, height=16
void drawBattery(int x, int y, int soc) {
  // Outer frame
  u8g2.drawFrame(x, y, 34, 14);
  // Tip
  u8g2.drawBox(x + 34, y + 4, 3, 6);
  // Fill
  int fill = (int)(32.0 * soc / 100.0);
  if (fill > 0)
    u8g2.drawBox(x + 1, y + 1, fill, 12);
  // If full invert text inside
  if (soc > 20) {
    u8g2.setDrawColor(0); // black text on white fill
  } else {
    u8g2.setDrawColor(1);
  }
  u8g2.setDrawColor(1); // reset
}

// Lightning bolt using lines (charging symbol)
void drawLightning(int x, int y) {
  // Simple lightning bolt shape using drawLine
  u8g2.drawLine(x + 6, y,     x + 2, y + 7);
  u8g2.drawLine(x + 2, y + 7, x + 5, y + 7);
  u8g2.drawLine(x + 5, y + 7, x,     y + 14);
  u8g2.drawLine(x,     y + 14,x + 4, y + 8);
  u8g2.drawLine(x + 4, y + 8, x + 1, y + 8);
  u8g2.drawLine(x + 1, y + 8, x + 6, y);
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  u8g2.begin();
  u8g2.setPowerSave(0);

  if (!ina260.begin()) {
    Serial.println("INA260 not found!");
    u8g2.firstPage();
    do {
      u8g2.setFont(u8g2_font_6x12_tf);
      u8g2.drawStr(0, 13, "INA260 NOT FOUND");
      u8g2.drawStr(0, 28, "SDA: A4");
      u8g2.drawStr(0, 43, "SCL: A5");
    } while (u8g2.nextPage());
    while (1);
  }

  Serial.println("==============================");
  Serial.println("   Power Station Monitor");
  Serial.println("==============================");
  Serial.println("VOLT    CURR     PWR     BAT");
  Serial.println("------------------------------");
}

void loop() {
  float voltage    = ina260.readBusVoltage() / 1000.0;
  float current    = ina260.readCurrent();
  float power      = ina260.readPower();
  int   soc        = calcSoC(voltage);
  bool  charging   = current < 0;
  float absCurrent = charging ? -current : current;

  char voltStr[8], currStr[8], pwrStr[8];
  dtostrf(voltage,        5, 2, voltStr);
  dtostrf(absCurrent,     6, 1, currStr);
  dtostrf(power / 1000.0, 4, 2, pwrStr);

  char socBuf[10], line2[20], line3[20], line4[20], line5[20];
  snprintf(socBuf, sizeof(socBuf), "%d PCT", soc);
  snprintf(line2,  sizeof(line2),  "VOLT: %sV",  voltStr);

  if (absCurrent >= 1000) {
    char currAStr[8];
    dtostrf(absCurrent / 1000.0, 4, 2, currAStr);
    snprintf(line3, sizeof(line3), "CURR: %sA",  currAStr);
  } else {
    snprintf(line3, sizeof(line3), "CURR: %smA", currStr);
  }

  snprintf(line4, sizeof(line4), "PWR : %sW", pwrStr);

  if (charging)
    snprintf(line5, sizeof(line5), "STATUS: CHARGING");
  else
    snprintf(line5, sizeof(line5), "STATUS: LOAD");

  // Serial
  Serial.print(voltStr); Serial.print("V    ");
  Serial.print(currStr); Serial.print("mA    ");
  Serial.print(pwrStr);  Serial.print("W    ");
  Serial.print(soc);     Serial.println(" PCT");

  // OLED
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_6x12_tf);

    // Row 1: battery symbol + SOC + lightning if charging
    drawBattery(0, 0, soc);
    u8g2.drawStr(42, 12, socBuf);
    if (charging)
      drawLightning(110, 0);

    // Divider
    u8g2.drawHLine(0, 15, 128);

    // Rows 2-5: readings
    u8g2.drawStr(0, 27, line2);
    u8g2.drawStr(0, 39, line3);
    u8g2.drawStr(0, 51, line4);
    u8g2.drawStr(0, 63, line5);

  } while (u8g2.nextPage());

  delay(500);
}