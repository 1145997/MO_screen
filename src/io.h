#pragma once
#include <stdint.h>

void io_init();
void io_service();

// 示例：继电器控制（可选用）
void io_relay_set(uint8_t index, bool on);