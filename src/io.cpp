#include "io.h"
#include <Arduino.h>

// 示例：两个继电器
static const uint8_t RELAY_PINS[] = { 10, 11 };
static constexpr uint8_t RELAY_N = sizeof(RELAY_PINS) / sizeof(RELAY_PINS[0]);

static bool g_relay[RELAY_N] = {false};

void io_init() {
  for (uint8_t i = 0; i < RELAY_N; i++) {
    pinMode(RELAY_PINS[i], OUTPUT);
    digitalWrite(RELAY_PINS[i], LOW);
  }
}

void io_relay_set(uint8_t index, bool on) {
  if (index >= RELAY_N) return;
  g_relay[index] = on;
  digitalWrite(RELAY_PINS[index], on ? HIGH : LOW);
}

void io_service() {
  // 预留：非阻塞任务、超时关闭、保护逻辑等
}