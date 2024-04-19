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
  vTaskDelay(pdMS_TO_TICKS(4000));
  xTaskCreate(time_sync, "syncs the time", 4096, NULL, 2, NULL  );
  xTaskCreate(free_space, "free space", 4096, NULL, 2, NULL  );
  dht_init();
  mpu6050_init();
  server_init();
  mdns_service();
}
