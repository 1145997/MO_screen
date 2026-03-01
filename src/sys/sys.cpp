#include "sys.h"

void sys_init() {
  Serial.begin(115200);
  om_wifi_init();
}

void sys_service() {
  om_wifi_service();
  om_status_service();
}


