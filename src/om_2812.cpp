#include "om_2812.h"
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>

// ======================
// 硬件配置
// ======================
static constexpr uint8_t DATA_PIN = 36;   // 你说的 35
static constexpr uint8_t W = 16;
static constexpr uint8_t H = 32;

// 你的灯板走线大概率是 ZIGZAG，且从左上角开始；不对就改 flags
static constexpr uint8_t MATRIX_FLAGS =
  NEO_MATRIX_TOP + NEO_MATRIX_LEFT +
  NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG;

static Adafruit_NeoMatrix matrix(
  W, H, DATA_PIN,
  MATRIX_FLAGS,
  NEO_GRB + NEO_KHZ800
);

// ======================
// 业务状态
// ======================
static Om2812Mode g_mode = OM2812_MODE_OFF;

// timing
static uint32_t g_tNext = 0;
static uint32_t g_tAnim = 0;

// for effects
static bool g_strobeOn = false;
static uint16_t g_rainbowHue = 0;
static int g_breath = 0;
static int g_breathDir = 1;

static uint16_t cRGB(uint8_t r, uint8_t g, uint8_t b) {
  return matrix.Color(r, g, b);
}

// 简单 HSV->RGB（够用）
static void hsv2rgb(uint16_t h, uint8_t s, uint8_t v, uint8_t &r, uint8_t &g, uint8_t &b) {
  // h:0..1535 (6*256-1)
  uint8_t region = (h >> 8) % 6;
  uint8_t f = h & 0xFF;
  uint16_t p = (uint16_t)v * (255 - s) / 255;
  uint16_t q = (uint16_t)v * (255 - ((uint16_t)s * f) / 255) / 255;
  uint16_t t = (uint16_t)v * (255 - ((uint16_t)s * (255 - f)) / 255) / 255;

  switch (region) {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    default:r = v; g = p; b = q; break;
  }
}

// ======================
// 模式控制
// ======================
void om2812_set_mode(Om2812Mode m) {
  if (m >= OM2812_MODE_N) m = OM2812_MODE_OFF;
  g_mode = m;

  // 重置动画参数
  g_tNext = 0;
  g_tAnim = 0;
  g_strobeOn = false;
  g_rainbowHue = 0;
  g_breath = 0;
  g_breathDir = 1;

  // 立刻清屏
  matrix.fillScreen(0);
  matrix.show();
}

Om2812Mode om2812_get_mode() { return g_mode; }

void om2812_next_mode() {
  uint8_t m = (uint8_t)g_mode;
  m = (m + 1) % OM2812_MODE_N;
  om2812_set_mode((Om2812Mode)m);
}

void om2812_prev_mode() {
  uint8_t m = (uint8_t)g_mode;
  m = (m == 0) ? (OM2812_MODE_N - 1) : (m - 1);
  om2812_set_mode((Om2812Mode)m);
}

// ======================
// init / service
// ======================
void om2812_init() {
  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setBrightness(40); // 512颗别拉太满，先稳
  matrix.fillScreen(0);
  matrix.show();
}

static void fx_off() {
  matrix.fillScreen(0);
  matrix.show();
}

static void fx_red_strobe(uint32_t now) {
  // 红色爆闪：50ms 亮/50ms 灭
  if (now < g_tNext) return;
  g_tNext = now + 50;

  g_strobeOn = !g_strobeOn;
  matrix.fillScreen(g_strobeOn ? cRGB(255, 0, 0) : 0);
  matrix.show();
}

static void fx_red_glitch(uint32_t now) {
  // 红色乱码：每 60ms 随机打点 + 随机短横
  if (now < g_tNext) return;
  g_tNext = now + 60;

  // 轻微拖影：不全清，做“乱码残留”
  // 先整体压暗（简化做法：随机擦一点）
  for (int i = 0; i < 20; i++) {
    int x = random(0, W);
    int y = random(0, H);
    matrix.drawPixel(x, y, 0);
  }

  // 随机红点
  for (int i = 0; i < 80; i++) {
    int x = random(0, W);
    int y = random(0, H);
    uint8_t r = random(80, 255);
    matrix.drawPixel(x, y, cRGB(r, 0, 0));
  }

  // 随机短横
  for (int k = 0; k < 6; k++) {
    int y = random(0, H);
    int x0 = random(0, W);
    int len = random(2, 8);
    uint8_t r = random(120, 255);
    for (int dx = 0; dx < len; dx++) {
      int x = x0 + dx;
      if (x >= 0 && x < W) matrix.drawPixel(x, y, cRGB(r, 0, 0));
    }
  }

  matrix.show();
}

static void fx_green_text(uint32_t now) {
  // 绿色文字：滚动 “ONLINE” + 时间戳感
  if (now < g_tNext) return;
  g_tNext = now + 45;

  static int16_t x = W;
  static uint32_t lastReset = 0;

  // 每隔几秒换一句（简单）
  if (now - lastReset > 6000) {
    lastReset = now;
    x = W;
  }

  matrix.fillScreen(0);
  matrix.setCursor(x, 10); // y 固定在中间偏上
  matrix.setTextColor(cRGB(0, 255, 0));
  matrix.print("ONLINE");

  // 第二行小字
  matrix.setCursor(x, 22);
  matrix.setTextColor(cRGB(0, 180, 0));
  matrix.print("READY");

  x--;
  int16_t w = 6 * 6; // “ONLINE”大概 6 字符 * 6 像素宽（含间距），粗估
  if (x < -w - 20) x = W;

  matrix.show();
}

static void fx_rainbow(uint32_t now) {
  // 彩虹滚动：每 30ms 刷一帧
  if (now < g_tNext) return;
  g_tNext = now + 30;

  for (uint8_t y = 0; y < H; y++) {
    for (uint8_t x = 0; x < W; x++) {
      uint16_t h = g_rainbowHue + (x * 40) + (y * 10);
      uint8_t r, g, b;
      hsv2rgb(h, 255, 120, r, g, b);
      matrix.drawPixel(x, y, cRGB(r, g, b));
    }
  }
  g_rainbowHue += 12;
  matrix.show();
}

static void fx_white_breath(uint32_t now) {
  // 白色呼吸：每 20ms 改一次亮度
  if (now < g_tNext) return;
  g_tNext = now + 20;

  g_breath += g_breathDir * 4;
  if (g_breath >= 255) { g_breath = 255; g_breathDir = -1; }
  if (g_breath <= 0)   { g_breath = 0;   g_breathDir =  1; }

  uint8_t v = (uint8_t)g_breath;
  matrix.fillScreen(cRGB(v, v, v));
  matrix.show();
}

static void fx_white_solid() {
  matrix.fillScreen(cRGB(255, 255, 255));
  matrix.show();
}

void om2812_service() {
  uint32_t now = millis();

  switch (g_mode) {
    case OM2812_MODE_OFF:         fx_off(); break;
    case OM2812_MODE_RED_STROBE:  fx_red_strobe(now); break;
    case OM2812_MODE_RED_GLITCH:  fx_red_glitch(now); break;
    case OM2812_MODE_GREEN_TEXT:  fx_green_text(now); break;
    case OM2812_MODE_RAINBOW:     fx_rainbow(now); break;
    case OM2812_MODE_WHITE_BREATH:fx_white_breath(now); break;
    case OM2812_MODE_WHITE_SOLID: fx_white_solid(); break;
    default: fx_off(); break;
  }
}