#include "protocol_examples_common.h"
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
#include <esp_err.h>
#include <driver/gpio.h>

typedef struct {
  char *buffer;
  int buffer_index;
} chunk_payload_t;

static char *buffer;
cJSON* info_buffer;

#define MASTER_FREQ 400000
#define MASTER_TIMEOUT 1000
#define REGISTER_READ_AMOUNT 7

#define MASTER_PORT1 I2C_NUM_1
#define CHP_SDA1 18
#define CHP_SCL1 19
#define CHP_ADDR1 0x68

esp_err_t i2c_read(uint8_t chp_addr, uint8_t port, uint8_t data_addr, uint8_t* data){
  i2c_cmd_handle_t cmd_handle=i2c_cmd_link_create();
  i2c_master_start(cmd_handle);
  i2c_master_write_byte(cmd_handle, (chp_addr << 1) | I2C_MASTER_WRITE, 1);
  i2c_master_write_byte(cmd_handle, data_addr, 1);
  i2c_master_start(cmd_handle);
  i2c_master_write_byte(cmd_handle, (chp_addr << 1) | I2C_MASTER_READ, 1);
  i2c_master_read(cmd_handle, data, REGISTER_READ_AMOUNT, I2C_MASTER_ACK);
  i2c_master_stop(cmd_handle);
  return(i2c_master_cmd_begin(port, cmd_handle, portMAX_DELAY));
  i2c_cmd_link_delete(cmd_handle);
}

esp_err_t i2c_write(uint8_t chp_addr, uint8_t port, uint8_t data_addr, uint8_t data){
  i2c_cmd_handle_t cmd_handle=i2c_cmd_link_create();
  i2c_master_start(cmd_handle); i2c_master_write_byte(cmd_handle, (chp_addr << 1) | I2C_MASTER_WRITE, 1);
  i2c_master_write_byte(cmd_handle, data_addr, 1);
  i2c_master_write_byte(cmd_handle, data, 1);
  i2c_master_stop(cmd_handle);
  return(i2c_master_cmd_begin(port, cmd_handle,portMAX_DELAY));
  i2c_cmd_link_delete(cmd_handle);
}

esp_err_t i2c_init(uint8_t sda, uint8_t scl, uint8_t port){
  i2c_config_t i2c_conf={
    .mode = I2C_MODE_MASTER,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_io_num = scl,
    .sda_io_num = sda,
    .master.clk_speed=400000,
  };  
  i2c_param_config(port, &i2c_conf);
  return(i2c_driver_install(port, I2C_MODE_MASTER, 0, 0, 0));
}

// for getting the data from HTTPS
esp_err_t client_event(esp_http_client_event_t *evt) {
  chunk_payload_t *chunk = evt->user_data;
  switch (evt->event_id) {
    case HTTP_EVENT_ON_DATA:
      ESP_LOGI("DATA_SIZE", "%d", evt->data_len);
      // for receiving data in chuncks
      chunk->buffer =
        realloc(chunk->buffer, chunk->buffer_index + evt->data_len + 1);
      memcpy(&chunk->buffer[chunk->buffer_index], (uint8_t *)evt->data,
             evt->data_len);
      chunk->buffer_index += evt->data_len;
      chunk->buffer[chunk->buffer_index] = 0;
      break;
    default:
      break;
  }
  buffer = chunk->buffer;
  return ESP_OK;
}

void wifi_init(){
  nvs_flash_init();
  esp_netif_init();
  esp_event_loop_create_default();
  example_connect();
}

void client_setup(){
  chunk_payload_t chunk = {0};
  esp_http_client_config_t client_config = {.url = "http://worldtimeapi.org/api/timezone/Asia/Kathmandu",
    .method = HTTP_METHOD_GET,
    .event_handler = client_event,
    .user_data = &chunk};
  esp_http_client_handle_t client_handle = esp_http_client_init(&client_config);
  esp_http_client_set_header(client_handle, "Content_type", "application/json");
  esp_http_client_perform(client_handle);
  info_buffer = cJSON_Parse(buffer);
  esp_http_client_cleanup(client_handle);
}

uint8_t char_to_hex(char pos_10, char pos_1){
    pos_10-='0';
    pos_1-='0';
    return((0x0f&pos_10)<<4)|(0x0f&pos_1);
}

void app_main(void) {
  wifi_init();
  client_setup();

  uint8_t sec=0;
  uint8_t min=0;
  uint8_t hr=0;
  uint8_t date=0;
  uint8_t month=0;
  uint8_t year=0;

  // getting the required data from json
  if (info_buffer == NULL) {
    printf("null\n\n\n");
  } else {
    cJSON *datetime =cJSON_GetObjectItemCaseSensitive(info_buffer, "datetime");
    // printf("\ndatetime: %s\n\n", datetime->valuestring);
    char* time=datetime->valuestring;

    hr= char_to_hex(time[11], time[12]);
    min= char_to_hex(time[14], time[15]);
    sec= char_to_hex(time[17], time[18]);
    date= char_to_hex(time[8], time[9]);
    month= char_to_hex(time[5], time[6]);
    year= char_to_hex(time[2], time[3]);
  }
  example_disconnect();

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
    printf("\nDATE: %x/%x/%x \n",data[6], data[5], data[4]);
    printf("TIME: %x:%x:%x \n",data[2], data[1], data[0]);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
