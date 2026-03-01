#include "om_wifi_rx.h"
#include <WiFi.h>

extern "C" {
  #include "esp_wifi.h"
}
#include <esp_now.h>

// extern DataPacket data;

static const uint8_t OM_WIFI_CH = 1;
static volatile uint32_t g_rxCount = 0;

static void onReceive(const uint8_t *mac, const uint8_t *incomingData, int len) {
  if (len != (int)sizeof(data)) {
    Serial.print("[RX] bad len=");
    Serial.println(len);
    return;
  }

  memcpy(&data, incomingData, sizeof(data));
  g_rxCount++;

  Serial.print("[RX] ");
  Serial.print(g_rxCount);
  Serial.print(" value=");
  Serial.println(data.value);
}

void om_wifi_init() {
  // Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  esp_wifi_set_channel(OM_WIFI_CH, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("[RX] ESP-NOW init failed");
    return;
  }

  esp_now_register_recv_cb(onReceive);
  Serial.println("[RX] ESP-NOW ready");
}

void om_wifi_service() {
  // 先空着也行。后面可加：失联保护、状态上报、ACK 等
}