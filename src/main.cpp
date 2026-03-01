#include <esp_now.h>
#include <WiFi.h>

extern "C" {
  #include "esp_wifi.h"
}

typedef struct {
  int value;
} DataPacket;

DataPacket data;

void onReceive(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&data, incomingData, sizeof(data));
  Serial.print("Received: ");
  Serial.println(data.value);
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);


  esp_now_init();
  esp_now_register_recv_cb(onReceive);
}

void loop() {
}