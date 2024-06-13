#include "Arduino_LED_Matrix.h"
#include "IPAddress.h"
#include "RTC.h"
#include "WiFiS3.h"
#include <ArduinoJson.h>
#include <Crypto.h>
#include <AES.h>
#include <string.h>
#include <EEPROM.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADS1X15.h>
#include <Adafruit_BME280.h>

#include "swps_secrets.h" 

#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10

ArduinoLEDMatrix matrix;
AES128 aes128;
WiFiClient client;
RTCTime currentTime;
Adafruit_ADS1115 ads;
Adafruit_BME280 bme(BME_CS);

char swps_sn[] = SWPS_SN;
int status = WL_IDLE_STATUS;
uint8_t ads_addr = 0x48;
long soil_moisture = 26000;
long pump_start_time = 500;
long detect_interval = 10;
bool detect_lock = false;

const uint32_t swps_logo[] = {
    0x77384967,
    0x147f581,
    0x56951cae
};

const uint32_t ledmat_off[] = {
    0x0,
    0x0,
    0x0
};

struct WiFiCfg {
    byte ssid[32];
    byte password[64];
    IPAddress serverip;
    uint16_t serverport;
};

JsonDocument create_data_doc(String api, int result){
    JsonDocument doc;
    doc["Api"] = api;
    doc["Result"] = result;

    return doc;
}

void set_params() {
    String msg;
    JsonDocument doc = create_data_doc("set_params", 0);
    JsonObject data = doc["Data"].to<JsonObject>();

    matrix.loadFrame(LEDMATRIX_CLOUD_WIFI);

    serializeJson(doc, msg);
    client.print(msg);

    while (!client.available()) {};
    msg = client.readStringUntil('\n');
    DeserializationError err = deserializeJson(doc, msg);

    matrix.loadFrame(ledmat_off);

    if (err.code() == DeserializationError::Ok) {
        if (doc["Result"] == 1) {
        detect_interval = doc["Data"]["DetectInterval"];
        pump_start_time = doc["Data"]["PumpStartTime"];
        soil_moisture = doc["Data"]["SoilMoisture"];
        double time_stamp = doc["Data"]["RTCTime"];
        RTCTime server_time = RTCTime(time_stamp);
        RTC.setTime(server_time);
        }
    }
}

bool upload_sensor_record() {
    RTCTime detect_time;
    String msg;
    JsonDocument doc = create_data_doc("upload_sensor_record", 0);
    JsonObject data = doc["Data"].to<JsonObject>();

    data["DeviceSN"] = swps_sn;

    data["Temperature"] = bme.readTemperature();
    data["Humidity"] = bme.readHumidity();
    data["Pressure"] = bme.readPressure() / 100.0F;

    int16_t adc0 = ads.readADC_SingleEnded(0);
    int16_t adc1 = ads.readADC_SingleEnded(1);
    int16_t adc2 = ads.readADC_SingleEnded(2);
    int16_t adc3 = ads.readADC_SingleEnded(3);
    
    data["RawValue0"] = adc0;
    data["RawValue1"] = adc1;
    data["RawValue2"] = adc2;
    data["RawValue3"] = adc3;
    data["Voltage0"] = ads.computeVolts(adc0);
    data["Voltage1"] = ads.computeVolts(adc1);
    data["Voltage2"] = ads.computeVolts(adc2);
    data["Voltage3"] = ads.computeVolts(adc3);

    RTC.getTime(detect_time);
    double time_stamp = detect_time.getUnixTime();
    data["DetectTime"] = time_stamp;

    data["PumpStartTime"] = start_water_pump(adc2);

    matrix.loadFrame(LEDMATRIX_CLOUD_WIFI);
    
    serializeJson(doc, msg);
    client.print(msg);

    while (!client.available()) {};
    msg = client.readStringUntil('\n');
    DeserializationError err = deserializeJson(doc, msg);

    matrix.loadFrame(ledmat_off);

    if (err.code() != DeserializationError::Ok){
        delay(2000);
        
        return false;
    }
    else if (doc["Result"] == 1) {
        return true;
    }
    else {
        delay(2000);

        return false;
    }
}

long start_water_pump(int16_t soil_raw) {
    long pump_runtime;

    if (soil_raw > soil_moisture) {
        pump_runtime = pump_start_time;

        digitalWrite(PIN_D8, HIGH);
        delay(pump_runtime);
        digitalWrite(PIN_D8, LOW);
    }
    else {
        pump_runtime = 0;
    }

    return pump_runtime;
}

void reset_wifi_cfg(String ssid, String pwd, String server_ip, uint16_t server_port) {
    digitalWrite(LED_BUILTIN, HIGH);

    write_wificfg_eeprom(0, ssid, pwd, server_ip, server_port);

    WiFiCfg wifi_cfg = read_wificfg_eeprom(0);

    digitalWrite(LED_BUILTIN, LOW);
    matrix.loadFrame(LEDMATRIX_EMOJI_HAPPY);

    delay(2000);

    NVIC_SystemReset();
    while (true);
}

void reconnect_wifi_client() {
    client.stop();
    matrix.loadFrame(LEDMATRIX_CLOUD_WIFI);

    digitalWrite(LED_BUILTIN, HIGH);
    WiFiCfg wifi_cfg = read_wificfg_eeprom(0);
    digitalWrite(LED_BUILTIN, LOW);

    // wait 10 seconds for connection:
    delay(10000);

    if (WiFi.status() != WL_CONNECTED) {
        matrix.loadFrame(LEDMATRIX_EMOJI_SAD);
        while (true);
    }

    client.connect(wifi_cfg.serverip, wifi_cfg.serverport);

    matrix.loadFrame(ledmat_off);
    delay(1000);
}

void write_wificfg_eeprom(int address, String ssid, String pwd, String server_ip, uint16_t server_port) {
    digitalWrite(LED_BUILTIN, HIGH);

    WiFiCfg cfg;
    uint8_t ssid_len = ssid.length() + 1;
    uint8_t pwd_len = pwd.length() + 1;
    ssid.getBytes(cfg.ssid, ssid_len);
    pwd.getBytes(cfg.password, pwd_len);
    aes128.encryptBlock(cfg.password, cfg.password);
    cfg.serverip.fromString(server_ip);
    cfg.serverport = server_port;

    EEPROM.put(address, cfg);

    digitalWrite(LED_BUILTIN, LOW);
}

WiFiCfg read_wificfg_eeprom(int address) {
    digitalWrite(LED_BUILTIN, HIGH);

    WiFiCfg cfg;
    EEPROM.get(0, cfg);
    aes128.decryptBlock(cfg.password, cfg.password);

    digitalWrite(LED_BUILTIN, LOW);

    return cfg;
}

void setup() {
    JsonDocument doc;
    String msg;
    uint8_t wifi_retry_times = 0;
    bool wifi_check = true;
    bool setup_check = false;

    Serial.begin(115200);
    matrix.begin();
    RTC.begin();
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(PIN_D8, OUTPUT);
    aes128.setKey(AES_KEY, 16);

    matrix.loadFrame(swps_logo);

    WiFiCfg wifi_cfg = read_wificfg_eeprom(0);

    if (!ads.begin(ads_addr)) {
        ads_addr++;
        if (ads_addr > 0x4B) {
            while (true);
        }
    }

    ads.setGain(GAIN_TWOTHIRDS);

    if (!bme.begin()) {
        while (true);
    }

    delay(2000);

    // check for the WiFi module:
    if (WiFi.status() == WL_NO_MODULE) {
        matrix.loadFrame(LEDMATRIX_DANGER);
        // don't continue
        while (true);
    }

    // check or set the WiFi config:
    while (String((char *)wifi_cfg.ssid) == "") {
        wifi_check = false;
    }

    // attempt to connect to WiFi network:
    while ((status != WL_CONNECTED) && (wifi_check)) {
        matrix.loadFrame(LEDMATRIX_CLOUD_WIFI);
        // Connect to WPA/WPA2 network.
        status = WiFi.begin((char *)wifi_cfg.ssid, (char *)wifi_cfg.password);
     
        // wait 10 seconds for connection:
        delay(10000);

        if (wifi_retry_times > 3) {
            matrix.loadFrame(LEDMATRIX_EMOJI_SAD);
            // don't continue
            wifi_check = false;
        }

        wifi_retry_times++;
    }

    wifi_retry_times = 0;

    while ((!client.connected()) && (wifi_check)) {
        matrix.loadFrame(swps_logo);

        client.connect(wifi_cfg.serverip, wifi_cfg.serverport);

        // wait 10 seconds for connection:
        delay(10000);

        if (wifi_retry_times > 3) {
            matrix.loadFrame(LEDMATRIX_EMOJI_SAD);
            // don't continue
            wifi_check = false;
        }

        wifi_retry_times++;
    }

    while (!wifi_check) {
        if (Serial.available() > 0) {
            msg = Serial.readStringUntil('\n');
            DeserializationError err = deserializeJson(doc, msg);

            if (err.code() != DeserializationError::Ok) {
                delay(100);
            }
            else if (doc["Api"] == "get_device_sn") {
                doc = create_data_doc("", 1);
                JsonObject data = doc["Data"].to<JsonObject>();
                data["DeviceSN"] = swps_sn;
                serializeJson(doc, msg);
                Serial.println(msg);
            }
            else if (doc["Api"] == "reset_wifi") {
                if (doc["Data"]["DeviceSN"] == swps_sn) {
                    String ssid = String(doc["Data"]["Ssid"]);
                    String pwd = String(doc["Data"]["Password"]);
                    String server_ip = String(doc["Data"]["ServerIP"]);
                    uint16_t server_port = doc["Data"]["ServerPort"];
                    reset_wifi_cfg(ssid, pwd, server_ip, server_port);
                }
            }
        }
    }

    while (!setup_check) {
        doc = create_data_doc("setup_edge", 1);
        JsonObject data = doc["Data"].to<JsonObject>();
        data["DeviceSN"] = swps_sn;

        matrix.loadFrame(LEDMATRIX_CLOUD_WIFI);

        serializeJson(doc, msg);
        client.print(msg);

        while (!client.available()) {};
        msg = client.readStringUntil('\n');
        DeserializationError err = deserializeJson(doc, msg);

        matrix.loadFrame(ledmat_off);

        if (err.code() != DeserializationError::Ok){
            setup_check = false;
        }
        else if (doc["Result"] == 1) {
            setup_check = true;
        }

        delay(2000);
    }
    
    set_params();

    matrix.loadFrame(ledmat_off);
}

void loop() {
    RTC.getTime(currentTime);

    if (Serial.available() > 0) {
        JsonDocument doc;
        String msg = Serial.readStringUntil('\n');
        DeserializationError err = deserializeJson(doc, msg);

        if (err.code() != DeserializationError::Ok) {
                delay(100);
        }
        else if (doc["Api"] == "get_device_sn") {
            doc = create_data_doc("", 1);
            JsonObject data = doc["Data"].to<JsonObject>();
            data["DeviceSN"] = swps_sn;
            serializeJson(doc, msg);
            Serial.println(msg);
        }
        else if (doc["Api"] == "reset_wifi") {
            if (doc["Data"]["DeviceSN"] == swps_sn) {
                String ssid = String(doc["Data"]["Ssid"]);
                String pwd = String(doc["Data"]["Password"]);
                String server_ip = String(doc["Data"]["ServerIP"]);
                uint16_t server_port = doc["Data"]["ServerPort"];
                reset_wifi_cfg(ssid, pwd, server_ip, server_port);
            }
        }
    }
    else if (!client.connected()) {
        reconnect_wifi_client();
    }
    else if ((currentTime.getMinutes() % detect_interval == 0) && (!detect_lock)) {
        set_params();
        detect_lock = upload_sensor_record();
        matrix.loadFrame(LEDMATRIX_HEART_SMALL);
    }
    else if (currentTime.getMinutes() % detect_interval != 0) {
        detect_lock = false;
        matrix.loadFrame(ledmat_off);
    }
}
