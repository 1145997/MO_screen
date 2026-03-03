#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// =====================
// 面板配置
// =====================
static constexpr uint8_t  PIN_WS = 41;     // WS2812 数据引脚
static constexpr uint16_t W = 16;
static constexpr uint16_t H = 32;
static constexpr uint16_t LED_N = W * H;

// =====================
// 映射开关（与你之前成功的那版一致）
// =====================
static constexpr bool MAP_SERPENTINE = true;  // true=行蛇形
static constexpr bool MAP_FLIP_X     = false;
static constexpr bool MAP_FLIP_Y     = false;

// =====================
// 按键配置：按下接 GND（LOW=pressed），使用 INPUT_PULLUP
// 你改这里 6 个 GPIO
// =====================
static const uint8_t BTN_PIN[6] = {
  35, 36, 37, 38, 39, 41
};

static constexpr uint32_t DEBOUNCE_MS = 25;
static constexpr bool ACTIVE_LOW = true;

// =====================
// 7个模式
// =====================
enum Mode : uint8_t {
  M_OFF = 0,
  M_RED_STROBE,
  M_RED_GLITCH,
  M_RED_BREATH,
  M_PURPLE_BREATH,
  M_GO_PORT_GREEN,
  M_RAINBOW,
  M_N
};

static const char* modeName(Mode m) {
  switch(m){
    case M_OFF: return "OFF";
    case M_RED_STROBE: return "RED_STROBE";
    case M_RED_GLITCH: return "RED_GLITCH";
    case M_RED_BREATH: return "RED_BREATH";
    case M_PURPLE_BREATH: return "PURPLE_BREATH";
    case M_GO_PORT_GREEN: return "GO_PORT_GREEN";
    case M_RAINBOW: return "RAINBOW";
    default: return "UNKNOWN";
  }
}

Adafruit_NeoPixel strip(LED_N, PIN_WS, NEO_GRB + NEO_KHZ800);

static inline uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return strip.Color(r, g, b);
}

// XY -> index（默认：左上角 (0,0)，按行）
static uint16_t XY(uint16_t x, uint16_t y) {
  if (MAP_FLIP_X) x = (W - 1) - x;
  if (MAP_FLIP_Y) y = (H - 1) - y;
  if (x >= W || y >= H) return 0;

  if (!MAP_SERPENTINE) return y * W + x;

  // serpentine rows
  if ((y & 1) == 0) return y * W + x;
  return y * W + (W - 1 - x);
}

static void clearAll() { strip.clear(); strip.show(); }

static void fillAll(uint8_t r, uint8_t g, uint8_t b) {
  uint32_t c = rgb(r,g,b);
  for (uint16_t i=0;i<LED_N;i++) strip.setPixelColor(i, c);
  strip.show();
}

// =====================
// 最小 5x7 字库：覆盖 GO PORT 所需字母 + 空格
// 低位在上（cy=0 顶部）
// =====================
struct Glyph { char ch; uint8_t col[5]; };

static const Glyph FONT[] = {
  {' ', {0x00,0x00,0x00,0x00,0x00}},
  {'G', {0x3E,0x41,0x49,0x49,0x2E}},
  {'O', {0x3E,0x41,0x41,0x41,0x3E}},
  {'P', {0x7F,0x09,0x09,0x09,0x06}},
  {'R', {0x7F,0x09,0x19,0x29,0x46}},
  {'T', {0x01,0x01,0x7F,0x01,0x01}},
};

static const Glyph* getGlyph(char c) {
  for (auto &g : FONT) if (g.ch == c) return &g;
  return &FONT[0];
}

static void drawChar5x7(int x, int y, char c, uint32_t color) {
  const Glyph* g = getGlyph(c);
  for (int cx=0; cx<5; cx++) {
    uint8_t bits = g->col[cx];
    for (int cy=0; cy<7; cy++) {
      if ((bits >> cy) & 0x01) {
        int px = x + cx;
        int py = y + cy;
        if (px>=0 && px<(int)W && py>=0 && py<(int)H) {
          strip.setPixelColor(XY(px, py), color);
        }
      }
    }
  }
}

static void drawText5x7(int x, int y, const char* s, uint32_t color) {
  int cursor = x;
  for (const char* p=s; *p; p++) {
    drawChar5x7(cursor, y, *p, color);
    cursor += 6; // 5 + 1 spacing
  }
}

// =====================
// 按键去抖（上升沿触发）
// =====================
struct BtnState {
  bool rawLast = false;
  bool stable = false;
  bool stableLast = false;
  uint32_t lastChangeMs = 0;
};

static BtnState g_btn[6];

static bool readPressed(uint8_t pin) {
  int v = digitalRead(pin);
  return ACTIVE_LOW ? (v == LOW) : (v == HIGH);
}

static bool btnPressedEdge(uint8_t i) {
  uint32_t now = millis();
  bool raw = readPressed(BTN_PIN[i]);

  if (raw != g_btn[i].rawLast) {
    g_btn[i].rawLast = raw;
    g_btn[i].lastChangeMs = now;
  }
  if ((now - g_btn[i].lastChangeMs) >= DEBOUNCE_MS) {
    g_btn[i].stable = raw;
  }

  bool edge = (g_btn[i].stable && !g_btn[i].stableLast);
  g_btn[i].stableLast = g_btn[i].stable;
  return edge;
}

// =====================
// 模式与效果
// =====================
static Mode g_mode = M_GO_PORT_GREEN;

// breath params
static int g_breath = 0;
static int g_breathDir = 1;

static uint16_t g_rainHue = 0;

static void setMode(Mode m) {
  if (m >= M_N) m = M_OFF;
  g_mode = m;
  g_breath = 0;
  g_breathDir = 1;
  g_rainHue = 0;
  Serial.printf("[MODE] %u %s\n", (unsigned)g_mode, modeName(g_mode));
}

// HSV->RGB（简化够用），h:0..1535
static void hsv2rgb(uint16_t h, uint8_t s, uint8_t v, uint8_t &r, uint8_t &g, uint8_t &b) {
  uint8_t region = (h >> 8) % 6;
  uint8_t f = h & 0xFF;
  uint16_t p = (uint16_t)v * (255 - s) / 255;
  uint16_t q = (uint16_t)v * (255 - ((uint16_t)s * f) / 255) / 255;
  uint16_t t = (uint16_t)v * (255 - ((uint16_t)s * (255 - f)) / 255) / 255;
  switch (region) {
    case 0: r=v; g=t; b=p; break;
    case 1: r=q; g=v; b=p; break;
    case 2: r=p; g=v; b=t; break;
    case 3: r=p; g=q; b=v; break;
    case 4: r=t; g=p; b=v; break;
    default:r=v; g=p; b=q; break;
  }
}

// 每次 loop 跑一帧（允许 delay，简单粗暴）
static void renderFrame() {
  switch (g_mode) {
    case M_OFF: {
      strip.clear();
      strip.show();
      delay(50);
    } break;

    case M_RED_STROBE: {
      static bool on = false;
      on = !on;
      fillAll(on ? 255 : 0, 0, 0);
      delay(50);
    } break;

    case M_RED_GLITCH: {
      // 残影擦点
      for (int i=0;i<40;i++) strip.setPixelColor(XY(random(W), random(H)), 0);

      // 红点
      for (int i=0;i<160;i++) {
        uint8_t r = random(60,255);
        strip.setPixelColor(XY(random(W), random(H)), rgb(r,0,0));
      }
      // 短横
      for (int k=0;k<10;k++) {
        uint16_t y = random(H);
        uint16_t x0 = random(W);
        uint8_t len = random(2, 10);
        uint8_t r = random(120,255);
        for (uint8_t dx=0; dx<len; dx++) {
          uint16_t x = x0 + dx;
          if (x < W) strip.setPixelColor(XY(x,y), rgb(r,0,0));
        }
      }
      strip.show();
      delay(60);
    } break;

    case M_RED_BREATH:
    case M_PURPLE_BREATH: {
      g_breath += g_breathDir * 4;
      if (g_breath >= 255) { g_breath = 255; g_breathDir = -1; }
      if (g_breath <= 0)   { g_breath = 0;   g_breathDir =  1; }

      uint8_t v = (uint8_t)g_breath;

      if (g_mode == M_RED_BREATH) {
        fillAll(v, 0, 0);
      } else {
        // 紫：R+B
        fillAll(v, 0, v);
      }
      delay(20);
    } break;

    case M_GO_PORT_GREEN: {
      strip.clear();
      // 居中：文本宽 6*7=42px，屏宽16放不下，所以分两行显示
      // GO
      drawText5x7(2, 6, "GO", rgb(0,255,0));
      // PORT
      drawText5x7(0, 18, "PORT", rgb(0,255,0));
      strip.show();
      delay(80);
    } break;

    case M_RAINBOW: {
      for (uint16_t y=0; y<H; y++) {
        for (uint16_t x=0; x<W; x++) {
          uint16_t h = g_rainHue + x*40 + y*10;
          uint8_t r,g,b;
          hsv2rgb(h, 255, 120, r,g,b);
          strip.setPixelColor(XY(x,y), rgb(r,g,b));
        }
      }
      strip.show();
      g_rainHue += 12;
      delay(30);
    } break;

    default: break;
  }
}

// =====================
// 按键映射
// BTN1 上一，BTN2 下一，BTN3 红爆闪，BTN4 红乱码，BTN5 红呼吸，BTN6 紫呼吸
// =====================
static void handleButtons() {
  if (btnPressedEdge(0)) { // prev
    setMode((Mode)((g_mode == 0) ? (M_N - 1) : (g_mode - 1)));
  }
  if (btnPressedEdge(1)) { // next
    setMode((Mode)((g_mode + 1) % M_N));
  }
  if (btnPressedEdge(2)) setMode(M_RED_STROBE);
  if (btnPressedEdge(3)) setMode(M_RED_GLITCH);
  if (btnPressedEdge(4)) setMode(M_RED_BREATH);
  if (btnPressedEdge(5)) setMode(M_PURPLE_BREATH);
}

void setup() {
  Serial.begin(115200);
  delay(100);

  strip.begin();
  strip.setBrightness(40);
  strip.clear();
  strip.show();

  randomSeed(esp_random());

  for (int i=0;i<6;i++) {
    pinMode(BTN_PIN[i], INPUT_PULLUP);
    bool p = readPressed(BTN_PIN[i]);
    g_btn[i].rawLast = p;
    g_btn[i].stable = p;
    g_btn[i].stableLast = p;
    g_btn[i].lastChangeMs = millis();
  }

  // 上电默认显示 GO PORT
  setMode(M_GO_PORT_GREEN);

  // 自检闪一下
  fillAll(255,0,0); delay(150);
  fillAll(0,255,0); delay(150);
  fillAll(0,0,255); delay(150);
  clearAll();
}

void loop() {
  handleButtons();
  renderFrame();
}