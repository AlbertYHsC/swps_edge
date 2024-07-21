#ifndef COMM_H
#define COMM_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <Crypto.h>
#include <AES.h>
#include <string.h>
#include "Arduino_LED_Matrix.h"
#include "IPAddress.h"
#include "WiFiS3.h"

#include "swps_secrets.h"
#include "utils.h"

struct WiFiCfg {
    byte ssid[32];
    byte password[64];
    IPAddress serverip;
    uint16_t serverport;
};

void setup_comm();
void handle_serial();
bool handle_wifi(JsonDocument *doc);
void reset_wifi_cfg(String ssid, String pwd, String server_ip, uint16_t server_port);
void reconnect_wifi_client();
void write_wificfg_eeprom(int address, String ssid, String pwd, String server_ip, uint16_t server_port);
WiFiCfg read_wificfg_eeprom(int address);

#endif