#include <freertos/FreeRTOS.h>
#include <esp_log.h>
#include <esp_err.h>

// Custom libraries
#include <i2c_rw.h>
#include <client.h>
#include <server.h>
#include <sensors.h>
#include <connect.h>

void wifi_init(){
  connect_init();
  connect_sta("nepaldigisys", "NDS_0ffice", 10000);
}

void free_space(){
  while(1){
    int DRam = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    ESP_LOGI("FREE SPACE", "DRAM \t\t %d", DRam);
    vTaskDelay(pdMS_TO_TICKS(4000));
  }
}


void app_main(void) {
  wifi_init();
  vTaskDelay(pdMS_TO_TICKS(2000));
  
  tsl2561_init();
  vTaskDelay(pdMS_TO_TICKS(100));
  mpu6050_init();
  vTaskDelay(pdMS_TO_TICKS(100));
  tinyRTC_init();
  vTaskDelay(pdMS_TO_TICKS(100));

  dht_init();
  xTaskCreate(free_space, "free space", 4096, NULL, 2, NULL  );

  mdns_service();
  server_init();
}
