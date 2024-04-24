#include <freertos/FreeRTOS.h>
#include <esp_log.h>
#include <esp_err.h>

// Custom libraries
#include <i2c_rw.h>
#include <client.h>
#include <server.h>
#include <sensors.h>
#include <connect.h>
#include <mqtt.h>

void app_main(void) {
  if(wifi_init() == ESP_OK){
    mqtt_init();

    dht_init();

    tsl2561_init();
    mpu6050_init();
    tinyRTC_init();

    mdns_service();
    server_init();
  }
}
