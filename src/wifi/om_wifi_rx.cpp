#include "om_wifi_rx.h"
#include <WiFi.h>
#include "om_status_led.h"

extern "C" {
  #include "esp_wifi.h"
}
#include <esp_now.h>

typedef struct {
  int value;
} DataPacket;

DataPacket data;


static const uint8_t OM_WIFI_CH = 1;
static volatile uint32_t g_rxCount = 0;

void onReceive(const uint8_t * mac, const uint8_t *incomingData, int len) {
  Serial.printf("[RX] from %02X:%02X:%02X:%02X:%02X:%02X len=%d\n",
    mac[0],mac[1],mac[2],mac[3],mac[4],mac[5], len);

  if (len != sizeof(data)) return;

  memcpy(&data, incomingData, sizeof(data));

  om_status_pulse_rx();
  om_status_mark_link_alive();
  om_status_set_link_state(OM_LINK_CONNECTED);

  Serial.print("Received: ");
  Serial.println(data.value);
}

void om_wifi_init() {
  // Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  WiFi.setSleep(false);
  esp_wifi_set_ps(WIFI_PS_NONE);
  esp_wifi_start();
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  if (esp_now_init() != ESP_OK) {
    Serial.println("[RX] ESP-NOW init failed");
    return;
  }

  esp_now_register_recv_cb(onReceive);
  Serial.println("[RX] ESP-NOW ready");

  om_status_init(40);
  om_status_set_link_state(OM_LINK_READY);
  om_status_set_lost_timeout(600); // 你遥控频率高就改小，比如 300ms

}

void om_wifi_service() {
  // 先空着也行。后面可加：失联保护、状态上报、ACK 等
}