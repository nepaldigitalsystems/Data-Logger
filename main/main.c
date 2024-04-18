#include <connect.h>
#include <cJSON.h>
#include <esp_event.h>
#include <esp_http_client.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <stdio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include "driver/i2c.h"
#include "freertos/projdefs.h"
#include <esp_err.h>
#include <driver/gpio.h>

// Custom libraries
#include <i2c_rw.h>
#include <client.h>
#include <server.h>

#define REGISTER_READ_AMOUNT 7
#define MASTER_PORT1 I2C_NUM_1
#define CHP_SDA1 18
#define CHP_SCL1 19
#define CHP_ADDR1 0x68

void wifi_init(){
  connect_init();
  connect_sta("nepaldigisys", "NDS_0ffice", 10000);
}

uint8_t char_to_hex(char pos_10, char pos_1){
  pos_10-='0';
  pos_1-='0';
  return((0x0f&pos_10)<<4)|(0x0f&pos_1);
}

void time_sync(){
    uint8_t sec=0;
    uint8_t min=0;
    uint8_t hr=0;
    uint8_t date=0;
    uint8_t month=0;
    uint8_t year=0;

    if(rtc_client_setup()!=0){
      char* time = rtc_client_setup();

      hr= char_to_hex(time[11], time[12]);
      min= char_to_hex(time[14], time[15]);
      sec= char_to_hex(time[17], time[18]);
      date= char_to_hex(time[8], time[9]);
      month= char_to_hex(time[5], time[6]);
      year= char_to_hex(time[2], time[3]);

      uint8_t data[REGISTER_READ_AMOUNT]={};

      i2c_init(CHP_SDA1, CHP_SCL1, MASTER_PORT1);
      i2c_write(CHP_ADDR1, MASTER_PORT1, 0x07, 0xb3);
      i2c_write(CHP_ADDR1, MASTER_PORT1, 0x00, sec);
      i2c_write(CHP_ADDR1, MASTER_PORT1, 0x01, min);
      i2c_write(CHP_ADDR1, MASTER_PORT1, 0x02, hr);
      i2c_write(CHP_ADDR1, MASTER_PORT1, 0x04, date);
      i2c_write(CHP_ADDR1, MASTER_PORT1, 0x05, month);
      i2c_write(CHP_ADDR1, MASTER_PORT1, 0x06, year);

      while(1){
        i2c_read(CHP_ADDR1, MASTER_PORT1, 0x00, data);

        time_data[0]= data[0]; //sec
        time_data[1]= data[1]; //min
        time_data[2]= data[2]; //hr
        time_data[3]= data[4]; //date
        time_data[4]= data[5]; //month
        time_data[5]= data[6]; //year

        // printf("\nDATE: %2.2x/%2.2x/%2.2x \n",data[6], data[5], data[4]);
        // printf("TIME: %2.2x:%2.2x:%2.2x \n",data[2], data[1], data[0]);
        vTaskDelay(pdMS_TO_TICKS(1000));
      }
    }else{
      ESP_LOGE("TIME", "INVALID TIME");
        vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void free_space(){
  while(1){
    int DRam = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    ESP_LOGI("FREE SPACE", "DRAM \t\t %d", DRam);
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

void app_main(void) {
  wifi_init();
  vTaskDelay(pdMS_TO_TICKS(4000));
  xTaskCreate(time_sync, "syncs the time", 4096, NULL, 2, NULL  );
  xTaskCreate(free_space, "free space", 4096, NULL, 2, NULL  );
  // mdns_service();
  server_init();
}
