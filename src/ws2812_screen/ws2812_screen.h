#pragma
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>

// 初始化矩阵
void ws2812_init(uint8_t pin, uint8_t width, uint8_t height);

// 业务配置结构体
typedef struct {
  uint32_t start_time;    // 启动时间（用于非阻塞延时）
  uint8_t running;        // 任务是否正在运行
  uint32_t interval;      // 时间间隔
} ws2812_task_t;

// 非阻塞任务
void ws2812_task_full_red_flash(ws2812_task_t *task);
void ws2812_task_garbled(ws2812_task_t *task);
void ws2812_task_show_chr(ws2812_task_t *task, char input);

// 注册动作
void ws2812_register_task(void (*task_func)(), uint32_t interval);

// 播放器函数，检查是否有正在运行的任务
bool ws2812_is_service_active(void);

