#ifndef  SENSORS_H
#define  SENSORS_H

void time_sync();
void dht_init();
float get_humidity();
float get_temperature();
char* get_accel();
char* get_gyro();
void mpu6050_init();

#endif
