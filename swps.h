#ifndef SWPS_H
#define SWPS_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADS1X15.h>
#include <Adafruit_BME280.h>
#include "RTC.h"

#include "swps_secrets.h"
#include "comm.h"
#include "utils.h"

extern long detect_interval;

void setup_swps();
void set_params();
bool upload_sensor_record();
long start_water_pump(int16_t soil_raw);

#endif