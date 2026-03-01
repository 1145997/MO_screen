#include "sys/sys.h"


void setup() {
  sys_init();
}

void loop() {
  sys_service();
}