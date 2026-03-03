#include "btn.h"
#include <Arduino.h>
#include "om_2812.h"

static constexpr bool ACTIVE_LOW = true;
static constexpr uint32_t DEBOUNCE_MS = 25;

// 你自己按板子接线改这里（示例）
static const uint8_t BTN_PIN[BTN_N] = {
  4,  5,  6,  7,  15,  16
};

struct BtnState {
  bool stable = false;       // 稳定态（true=pressed）
  bool lastStable = false;   // 上一次稳定态
  bool rawLast = false;
  uint32_t lastChangeMs = 0;
};

static BtnState g_btn[BTN_N];

static bool readPressed(uint8_t pin) {
  int v = digitalRead(pin);
  bool pressed = ACTIVE_LOW ? (v == LOW) : (v == HIGH);
  return pressed;
}

static void onPressed(BtnId id) {
  switch (id) {
    case BTN_1: om2812_prev_mode(); break;
    case BTN_2: om2812_next_mode(); break;
    case BTN_3: om2812_set_mode(OM2812_MODE_RED_STROBE); break;
    case BTN_4: om2812_set_mode(OM2812_MODE_RED_GLITCH); break;
    case BTN_5: om2812_set_mode(OM2812_MODE_GREEN_TEXT); break;
    case BTN_6: om2812_set_mode(OM2812_MODE_OFF); break;
    default: break;
  }
}

void btn_init() {
  for (uint8_t i = 0; i < BTN_N; i++) {
    pinMode(BTN_PIN[i], INPUT_PULLUP);
    bool p = readPressed(BTN_PIN[i]);
    g_btn[i].stable = p;
    g_btn[i].lastStable = p;
    g_btn[i].rawLast = p;
    g_btn[i].lastChangeMs = millis();
  }
}

void btn_handle() {
  uint32_t now = millis();

  for (uint8_t i = 0; i < BTN_N; i++) {
    bool raw = readPressed(BTN_PIN[i]);

    if (raw != g_btn[i].rawLast) {
      g_btn[i].rawLast = raw;
      g_btn[i].lastChangeMs = now;
    }

    // raw 连续保持 DEBOUNCE_MS 才认为稳定
    if ((now - g_btn[i].lastChangeMs) >= DEBOUNCE_MS) {
      g_btn[i].stable = raw;
    }

    // 触发：稳定态从未按->按下（上升沿）
    if (g_btn[i].stable && !g_btn[i].lastStable) {
      onPressed((BtnId)i);
    }

    g_btn[i].lastStable = g_btn[i].stable;
  }
}

void btn_service() {
  btn_handle();
}