#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "Arduino_LED_Matrix.h"

extern ArduinoLEDMatrix matrix;
extern const uint32_t swps_logo[3];
extern const uint32_t ledmat_off[3];

void setup_led();
void load_led(const uint32_t buffer[3]);
JsonDocument create_data_doc(String api, int result);

#endif