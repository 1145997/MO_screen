#include "ws2812_screen.h"

#define MATRIX_PIN 35        // 数据管脚
#define MATRIX_WIDTH 32     // 屏幕宽度
#define MATRIX_HEIGHT 8     // 屏幕高度
#define BRIGHTNESS 40       // 屏幕亮度

// 创建 NeoMatrix 实例
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(
  MATRIX_WIDTH, MATRIX_HEIGHT, MATRIX_PIN,
  NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG,
  NEO_GRB + NEO_KHZ800
);

// 当前注册的任务函数
void (*current_task_func)();
uint32_t last_task_time = 0; // 上次任务时间
ws2812_task_t current_task;

// 初始化 WS2812 显示器
void ws2812_init(uint8_t pin, uint8_t width, uint8_t height) {
  matrix.begin();
  matrix.setBrightness(BRIGHTNESS);
  matrix.setTextWrap(false);
}

// 显示红色闪烁（业务一）
void ws2812_task_full_red_flash(ws2812_task_t *task) {
  if (millis() - task->start_time >= task->interval) {
    task->start_time = millis();
    matrix.fillScreen(matrix.Color(255, 0, 0)); // 红色全屏
    matrix.show();
    delay(500);  // 红色闪烁
    matrix.fillScreen(0);  // 清空
    matrix.show();
  }
}

// 显示乱码（业务二）
void ws2812_task_garbled(ws2812_task_t *task) {
  if (millis() - task->start_time >= task->interval) {
    task->start_time = millis();
    matrix.fillScreen(random(0, 255));  // 随机乱码效果
    matrix.show();
    delay(100);  // 闪烁
    matrix.fillScreen(0);  // 清空
    matrix.show();
  }
}

// 显示字符函数
void ws2812_task_show_chr(ws2812_task_t *task, char input) {
  static int x = MATRIX_WIDTH;
  int charWidth = 6;  // 每个字符宽度

  matrix.fillScreen(0);  // 清屏
  matrix.setCursor(x, 0);
  matrix.print(input);  // 打印字符
  matrix.show();

  x--;  // 向左滚动字符
  if (x < -charWidth) {  // 如果字符超出屏幕，重新开始
    x = MATRIX_WIDTH;
  }
}

// 注册一个业务动作（放入循环调用）
void ws2812_register_task(void (*task_func)(), uint32_t interval) {
  current_task_func = task_func;
  current_task.interval = interval;
  current_task.start_time = millis();
  current_task.running = 1;
}

// 播放器函数：检查当前是否有业务在运行
bool ws2812_is_service_active() {
  return current_task.running;
}

// 每次循环调用检查任务并执行
void ws2812_loop() {
  if (current_task_func) {
    current_task_func();
  }
}

// void ws2812_massion() {

// }



