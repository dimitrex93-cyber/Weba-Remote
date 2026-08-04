#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

#include "config.h"
#include "types.h"
#include "Bitmaps.h"

class DisplayManager {
public:
  bool initialize() {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    display_.setI2CAddress(static_cast<uint8_t>(OLED_ADDR << 1));
    return display_.begin();
  }

  void sleep() {
    display_.setPowerSave(1);
  }

  void wake() {
    display_.setPowerSave(0);
  }

  void render(const RemoteState& state, bool linkAlive) {
    display_.clearBuffer();

    display_.setFont(u8g2_font_6x10_tr);
    display_.drawStr(0, 10, "Weba Remote C3");
    drawEspNowLinkIcon10x10(display_, 117, 1, linkAlive);

    if (!state.statusValid) {
      display_.drawStr(0, 26, "Warte auf Status...");
      display_.drawStr(0, 40, "Pruefe MAC/Channel");
    } else {
      char line[30];
      snprintf(line, sizeof(line), "Heat: %s", state.heatingActive ? "AN" : "AUS");
      display_.drawStr(0, 24, line);

      snprintf(line, sizeof(line), "Temp: %.1f C", state.innenTemperatur);
      display_.drawStr(0, 36, line);

      snprintf(line, sizeof(line), "Batt: %.2f V", state.batterieSpannung);
      display_.drawStr(0, 48, line);

      uint16_t minutes = state.restzeitS / 60;
      uint16_t seconds = state.restzeitS % 60;
      snprintf(line, sizeof(line), "Rest: %02u:%02u", minutes, seconds);
      // y=56 statt 60 (Review 04.08.2026): sonst überlappte die 6x10-Zeile
      // mit der 5x7-lastAction-Zeile (Baseline 64, belegt ab Pixel 57).
      display_.drawStr(0, 56, line);
    }

    display_.setFont(u8g2_font_5x7_tr);
    display_.drawStr(0, 64, state.lastAction);

    display_.sendBuffer();
  }

private:
  U8G2_SSD1306_128X64_NONAME_F_HW_I2C display_{U8G2_R0, U8X8_PIN_NONE};
};

#endif
