#ifndef BITMAPS_H
#define BITMAPS_H

#include <Arduino.h>
#include <U8g2lib.h>

static inline void drawEspNowLinkIcon10x10(U8G2& display, uint8_t x, uint8_t y, bool linked) {
  const uint8_t cx = x + 5;
  const uint8_t cy = y + 8;
  const uint8_t upper = U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT;

  display.drawCircle(cx, cy, 1, upper);
  display.drawCircle(cx, cy, 2, upper);
  display.drawCircle(cx, cy, 4, upper);

  if (linked) {
    display.drawDisc(cx, cy, 1, U8G2_DRAW_ALL);
  } else {
    display.drawPixel(cx, cy);
    display.drawLine(x + 1, y + 9, x + 8, y + 2);
  }
}

#endif
