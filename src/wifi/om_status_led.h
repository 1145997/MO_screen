#pragma once
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// ====== 你可以改这些 ======
#ifndef OM_STATUS_PIN
#define OM_STATUS_PIN 48
#endif

#ifndef OM_STATUS_LED_COUNT
#define OM_STATUS_LED_COUNT 1   // 你的板子如果是 1 颗灯就用 1；如果是灯带就改成 N
#endif

// 通信状态（你在业务里切换）
enum OmLinkState : uint8_t {
  OM_LINK_BOOT = 0,      // 上电/初始化中：蓝色慢闪
  OM_LINK_READY,         // ESP-NOW ready：青色常亮（或弱呼吸）
  OM_LINK_CONNECTED,     // 通信正常：绿色呼吸
  OM_LINK_LOST,          // 超时未收到：黄色呼吸
  OM_LINK_ERROR          // 错误：红色快闪
};

void om_status_init(uint8_t brightness = 40);
void om_status_set_link_state(OmLinkState st);

// 事件打点：在回调里调用
void om_status_pulse_tx();      // 发送一次：白色短闪
void om_status_pulse_rx();      // 接收一次：紫色短闪
void om_status_pulse_warn();    // 警告：橙色短闪（可选）
void om_status_pulse_error();   // 错误：红色短闪（同时可切 OM_LINK_ERROR）

// 在 loop / sys_service 里周期调用（非阻塞）
void om_status_service();

// （可选）用于“掉线判断”的时间戳接口：你也可以自己做
void om_status_mark_link_alive();     // RX 收到包时调用，更新最后活跃时间
void om_status_set_lost_timeout(uint32_t ms); // 默认 600ms