#include <i2c_rw.h>
#include <client.h>
#include <server.h>
#include <esp_log.h>
#include <driver/i2c.h>
#include <freertos/FreeRTOS.h>

#include <math.h>
#include <stdio.h>
#include <driver/gpio.h>
#include <freertos/task.h>
#include <string.h>
#include <freertos/semphr.h>
#include <esp_timer.h>


// -----------------------------[ I2C ]--------------------------------- //
#define CHP_SDA1 18
#define CHP_SCL1 19
#define MASTER_PORT1 I2C_NUM_1

#define CHP_SDA0 32
#define CHP_SCL0 33
#define MASTER_PORT0 I2C_NUM_0

// -----------------------------[ RTC ]--------------------------------- //

#define REGISTER_READ_AMOUNT 7
#define RTC_ADDR 0x68

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
      i2c_write(RTC_ADDR, MASTER_PORT1, 0x07, 0xb3);
      i2c_write(RTC_ADDR, MASTER_PORT1, 0x00, sec);
      i2c_write(RTC_ADDR, MASTER_PORT1, 0x01, min);
      i2c_write(RTC_ADDR, MASTER_PORT1, 0x02, hr);
      i2c_write(RTC_ADDR, MASTER_PORT1, 0x04, date);
      i2c_write(RTC_ADDR, MASTER_PORT1, 0x05, month);
      i2c_write(RTC_ADDR, MASTER_PORT1, 0x06, year);

      while(1){
        i2c_read(RTC_ADDR, MASTER_PORT1, 0x00, data);

        time_data[0]= data[0]; //sec
        time_data[1]= data[1]; //min
        time_data[2]= data[2]; //hr
        time_data[3]= data[4]; //date
        time_data[4]= data[5]; //month
        time_data[5]= data[6]; //year

        // printf("\nDATE: %2.2x/%2.2x/%2.2x \n",data[6], data[5], data[4]);
        // printf("TIME: %2.2x:%2.2x:%2.2x \n",data[2], data[1], data[0]);
        vTaskDelay(pdMS_TO_TICKS(2000));
      }
    }else{
      ESP_LOGE("TIME", "INVALID TIME");
        vTaskDelay(pdMS_TO_TICKS(2000));
  }
}



// -----------------------------[ DHT22 ]--------------------------------- //
#define DHT_TIMEOUT_ERROR -2
#define DHT_CHECKSUM_ERROR -1
#define DHT_OK 0

#define DHT 27
#define DHT_NEG 26

#define tag "DHT"

int pos_count=0;
int neg_count=0;

int pos_time[44];
int neg_time[44];

bool dht_data[40];
uint8_t data[5];

float humidity;
float temperature;

TaskHandle_t signal_handler;

// function prototype
void startSignal();
int process_signal();

float get_humidity(){ return humidity; }
float get_temperature(){ return temperature; }


static IRAM_ATTR void pos_intr(){
  pos_time[pos_count]=  esp_timer_get_time();
  pos_count++;
}

static IRAM_ATTR void neg_intr(){
  neg_time[neg_count]=  esp_timer_get_time();
  neg_count++;
}


void error_handler(int response){
  switch(response){
    case DHT_TIMEOUT_ERROR:
      ESP_LOGE(tag, "DHT_TIMEOUT_ERROR");
      break;
    case DHT_CHECKSUM_ERROR:
      ESP_LOGE(tag, "DHT_CHECKOUT_ERROR");
      break;
    case DHT_OK:
      // printf("Humidity:%4.2f\nTemperature:%4.2f", get_humidity(), get_temperature()); printf("\n\n");
      break;
    default:
      ESP_LOGE(tag, "DHT_UNKNOWN_ERROR");
  }
}

void startSignal(){
  while(1){
    gpio_isr_handler_remove(DHT);
    gpio_isr_handler_remove(DHT_NEG);
    gpio_pulldown_en(DHT);
    gpio_set_direction(DHT, GPIO_MODE_OUTPUT);
    gpio_set_direction(DHT_NEG, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT, 0);
    esp_rom_delay_us(2000);
    gpio_set_level(DHT, 1);
    esp_rom_delay_us(25);
    gpio_set_level(DHT, 0);
    gpio_set_direction(DHT, GPIO_MODE_INPUT);
    gpio_set_direction(DHT_NEG, GPIO_MODE_INPUT);
    gpio_isr_handler_add(DHT, pos_intr, NULL);
    gpio_isr_handler_add(DHT_NEG, neg_intr, NULL);
    vTaskDelay(pdMS_TO_TICKS(50));
    xTaskNotifyGive(signal_handler);
    vTaskDelay(pdMS_TO_TICKS(4000));
  }
}

int process_signal(){
  for(int i=0 ; i<41 ; i++){
    int utime = neg_time[i]-pos_time[i] ;
    if(utime>0){
      dht_data[i] = utime>60 ? 1 : 0; 
    }else{
      return DHT_TIMEOUT_ERROR;
    }
  }

  memset(data, 0, sizeof(data));

  for(int i=0 ; i<5 ; i++){
    for(int j=0 ; j<8 ; j++){
      int k=(i*8)+j;
      int l=7-(k%8);

      if(dht_data[k+1]) data[i]|=(1<<l);
    }
  }

  uint8_t checksum = (data[0]+data[1]+data[2]+data[3])& 0xFF;

  humidity = ((data[0]*0x100)+data[1])/10.0;
  temperature = ((data[2]*0x100)+data[3])/10.0;

  if(checksum!=data[4]) return DHT_CHECKSUM_ERROR;

  return ESP_OK;
}


void dht_output(){
  while(1){
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    error_handler(process_signal());
    pos_count=0;
    neg_count=0;
  }
}

void intr_init(){
  gpio_install_isr_service(0);

  gpio_config_t pos_pin={
    .intr_type = GPIO_INTR_POSEDGE,
    .pull_down_en = GPIO_PULLDOWN_ENABLE,
    .pin_bit_mask = (1ULL << DHT),
  };
  gpio_config(&pos_pin);
  gpio_isr_handler_add(DHT, pos_intr, NULL);

  gpio_config_t neg_pin={
    .intr_type = GPIO_INTR_NEGEDGE,
    .pull_down_en = GPIO_PULLDOWN_ENABLE,
    .pin_bit_mask = (1ULL << DHT_NEG),
  };

  gpio_config(&neg_pin);
  gpio_isr_handler_add(DHT_NEG, neg_intr, NULL);
}


void dht_init()
{
  intr_init();
  xTaskCreatePinnedToCore(startSignal, "Start Signal", 2048, NULL, 4, NULL, 1);
  xTaskCreate(dht_output, "output value", 2048, NULL, 2, &signal_handler);
}



// -----------------------------[ MPU6050 ]--------------------------------- //

#define MPU6050_ADDR 0x68

// #define WHO_AM_I 0x75
#define POW_MAG 0x6b

char accel[100]={};
char gyro[100]={};

float h2d(uint8_t* data){
     return (((data[0]<<8)|data[1])/65536.0)*360.0;
}

void mpu6050_conf(){
  uint8_t accel_x[2]={};
  uint8_t accel_y[2]={};
  uint8_t accel_z[2]={};
  uint8_t gyro_x[2]={};
  uint8_t gyro_y[2]={};
  uint8_t gyro_z[2]={};
  // uint8_t temp[2]={};
  // int16_t signed_temp=0;

  i2c_init(CHP_SDA0, CHP_SCL0, MASTER_PORT0);

  // i2c_read(WHO_AM_I, data);
  i2c_write(MPU6050_ADDR, MASTER_PORT0, POW_MAG, 0x04);
  i2c_write(MPU6050_ADDR, MASTER_PORT0, 0x28, 0xf8);
  // i2c_write(0x26, 0x08);
  ESP_LOGI("STATUS", "Wrote to power_mgr" );
  while(1){
    i2c_read(MPU6050_ADDR, MASTER_PORT0, 0x3b, accel_x);
    i2c_read(MPU6050_ADDR, MASTER_PORT0, 0x3d, accel_y);
    i2c_read(MPU6050_ADDR, MASTER_PORT0, 0x3f, accel_z);
    i2c_read(MPU6050_ADDR, MASTER_PORT0, 0x43, gyro_x);
    i2c_read(MPU6050_ADDR, MASTER_PORT0, 0x45, gyro_y);
    i2c_read(MPU6050_ADDR, MASTER_PORT0, 0x47, gyro_z);
    // i2c_read(MPU6050_ADDR, MASTER_PORT0, 0x41, temp);
    // signed_temp= (int16_t)((temp[0]<<8)|temp[1]);

    // printf("accel: %10.2f%10.2f%10.2f\n", h2d(accel_x), h2d(accel_y), h2d(accel_z));
    // printf("gyro: %11.2f%10.2f%10.2f\n", h2d(gyro_x), h2d(gyro_y), h2d(gyro_z));
    // printf("temp: %11.2f\n\n", (signed_temp/340.0)+36.53);

    snprintf(accel, sizeof(accel), "%03.0f | %03.0f | %03.0f", h2d(accel_x), h2d(accel_y), h2d(accel_z));
    snprintf(gyro, sizeof(gyro), "%03.0f | %03.0f | %03.0f", h2d(gyro_x), h2d(gyro_y), h2d(gyro_z));
    vTaskDelay(pdMS_TO_TICKS(4000));
  }
}

char* get_accel(){
   return accel;
}

char* get_gyro(){
   return gyro;
}

void mpu6050_init(){
  xTaskCreate(mpu6050_conf, "MPU6050", 2048, NULL, 2, NULL);
}


