#include <Arduino.h>
#include "sys.h"
#include "btn.h"
#include "om_2812.h"
#include "io.h"

void sys_init() {
  Serial.begin(115200);
  delay(50);

  io_init();
  om2812_init();
  btn_init();

  // 上电默认状态：绿色文字
  om2812_set_mode(OM2812_MODE_GREEN_TEXT);
}

void sys_service() {
  btn_service();
  io_service();
  om2812_service();
}