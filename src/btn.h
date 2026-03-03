#pragma once
#include <stdint.h>

enum BtnId : uint8_t {
  BTN_1 = 0,
  BTN_2,
  BTN_3,
  BTN_4,
  BTN_5,
  BTN_6,
  BTN_N
};

void btn_init();
void btn_handle();   // 单次轮询（一般不直接用）
void btn_service();  // 周期调用