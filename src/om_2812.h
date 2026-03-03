#pragma once
#include <stdint.h>

enum Om2812Mode : uint8_t {
  OM2812_MODE_OFF = 0,
  OM2812_MODE_RED_STROBE,
  OM2812_MODE_RED_GLITCH,
  OM2812_MODE_GREEN_TEXT,
  OM2812_MODE_RAINBOW,
  OM2812_MODE_WHITE_BREATH,
  OM2812_MODE_WHITE_SOLID,
  OM2812_MODE_N
};

void om2812_init();
void om2812_service();

void om2812_set_mode(Om2812Mode m);
Om2812Mode om2812_get_mode();

void om2812_next_mode();
void om2812_prev_mode();