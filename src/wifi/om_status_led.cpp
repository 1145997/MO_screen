#include "om_status_led.h"

// 你的库
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>

static Adafruit_NeoPixel g_led(OM_STATUS_LED_COUNT, OM_STATUS_PIN, NEO_GRB + NEO_KHZ800);

static OmLinkState g_state = OM_LINK_BOOT;

static uint32_t g_lastMs = 0;
static uint16_t g_phase = 0;

// 事件闪烁的“剩余时间”
static uint32_t g_txPulseUntil = 0;
static uint32_t g_rxPulseUntil = 0;
static uint32_t g_warnPulseUntil = 0;
static uint32_t g_errPulseUntil = 0;

// 掉线检测
static uint32_t g_lastAliveMs = 0;
static uint32_t g_lostTimeoutMs = 600;

static inline uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return g_led.Color(r, g, b);
}

static void showAll(uint32_t c) {
  for (uint16_t i = 0; i < OM_STATUS_LED_COUNT; i++) g_led.setPixelColor(i, c);
  g_led.show();
}

// 三角波呼吸 0..255
static uint8_t tri8(uint16_t p) {
  // p: 0..1023
  p &= 1023;
  if (p < 512) return (uint8_t)(p >> 1);            // 0..255
  return (uint8_t)((1023 - p) >> 1);                // 255..0
}

void om_status_init(uint8_t brightness) {
  g_led.begin();
  g_led.setBrightness(brightness);
  g_led.clear();
  g_led.show();

  g_state = OM_LINK_BOOT;
  g_lastMs = millis();
  g_phase = 0;

  g_txPulseUntil = g_rxPulseUntil = g_warnPulseUntil = g_errPulseUntil = 0;

  g_lastAliveMs = millis();
  g_lostTimeoutMs = 600;
}

void om_status_set_link_state(OmLinkState st) {
  g_state = st;
}

void om_status_pulse_tx() {
  g_txPulseUntil = millis() + 80;   // 80ms 白闪
}

void om_status_pulse_rx() {
  g_rxPulseUntil = millis() + 80;   // 80ms 紫闪
}

void om_status_pulse_warn() {
  g_warnPulseUntil = millis() + 120; // 120ms 橙闪
}

void om_status_pulse_error() {
  g_errPulseUntil = millis() + 150; // 150ms 红闪
}

void om_status_mark_link_alive() {
  g_lastAliveMs = millis();
}

void om_status_set_lost_timeout(uint32_t ms) {
  g_lostTimeoutMs = ms;
}

void om_status_service() {
  uint32_t now = millis();
  uint32_t dt = now - g_lastMs;
  g_lastMs = now;

  // 相位推进：不同状态不同速度
  // phase 是 0..1023 的呼吸相位
  uint16_t speed = 2; // 默认慢
  switch (g_state) {
    case OM_LINK_BOOT:      speed = 6;  break;  // 蓝慢闪（相位快一点更明显）
    case OM_LINK_READY:     speed = 2;  break;  // 青弱呼吸
    case OM_LINK_CONNECTED: speed = 3;  break;  // 绿呼吸
    case OM_LINK_LOST:      speed = 4;  break;  // 黄呼吸（略快）
    case OM_LINK_ERROR:     speed = 10; break;  // 红快闪
    default:                speed = 3;  break;
  }

  // 用 dt 推进（避免帧率变化导致速度变化）
  // 约每 10ms 增 1 步的感觉：这里做个比例
  uint16_t step = (uint16_t)max<uint32_t>(1, dt / 10);
  g_phase = (uint16_t)((g_phase + (uint16_t)(step * speed)) & 1023);

  // ===== 掉线自动切 LOST（可选：只对 RX 板更有意义）=====
  if (g_state == OM_LINK_CONNECTED || g_state == OM_LINK_READY) {
    if ((now - g_lastAliveMs) > g_lostTimeoutMs) {
      g_state = OM_LINK_LOST;
    }
  }

  // ===== 底色（状态色）=====
  uint8_t a = tri8(g_phase);               // 0..255
  uint8_t low = (uint8_t)(a / 3);          // 弱呼吸
  uint8_t mid = (uint8_t)(a / 2);          // 中呼吸

  uint32_t base = 0;

  switch (g_state) {
    case OM_LINK_BOOT:
      // 蓝色闪：用 tri 做亮灭
      base = rgb(0, 0, mid);
      break;

    case OM_LINK_READY:
      // 青色弱呼吸
      base = rgb(0, low, low);
      break;

    case OM_LINK_CONNECTED:
      // 绿色呼吸
      base = rgb(0, mid, 0);
      break;

    case OM_LINK_LOST:
      // 黄色呼吸
      base = rgb(mid, mid / 2, 0);
      break;

    case OM_LINK_ERROR:
      // 红快闪（用 tri 做闪烁）
      base = (a > 180) ? rgb(255, 0, 0) : rgb(0, 0, 0);
      break;
  }

  // ===== 事件覆盖层（优先级：error > warn > rx > tx）=====
  uint32_t overlay = 0;
  if (now < g_errPulseUntil)       overlay = rgb(255, 0, 0);
  else if (now < g_warnPulseUntil) overlay = rgb(255, 80, 0);
  else if (now < g_rxPulseUntil)   overlay = rgb(160, 0, 255);
  else if (now < g_txPulseUntil)   overlay = rgb(255, 255, 255);

  uint32_t out = overlay ? overlay : base;
  showAll(out);
}