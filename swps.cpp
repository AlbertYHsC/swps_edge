#include "swps.h"

Adafruit_ADS1115 ads;
Adafruit_BME280 bme;
static uint8_t ads_addr;
static uint8_t bme_addr;
static long soil_moisture = 26000;
static long pump_start_time = 500;
long detect_interval = 10;

void setup_swps() {
    bool ads_check = false; // Set true to bypass I2C device checking
    bool bme_check = false; // Set true to bypass I2C device checking

    RTC.begin();
    pinMode(PIN_D8, OUTPUT);
    Wire.setClock(400000);

    load_led(swps_logo);

    // The ADS1115 address range is from 0x48 to 0x4B
    for (ads_addr = 0x48; ads_addr <= 0x4B; ads_addr++) {
        if (ads.begin(ads_addr, &Wire)) {
            ads.setGain(GAIN_TWOTHIRDS);
            ads_check = true;
            break;
        }
    }

    // The BME280 address is 0x76 or 0x77
    for (bme_addr = 0x76; bme_addr <= 0x77; bme_addr++) {
        if (bme.begin(bme_addr, &Wire)) {
            bme_check = true;
            break;
        }
    }

    if (!(ads_check || bme_check)) {
        load_led(LEDMATRIX_DANGER);
        while (true);
    }

    delay(2000); // Display SWPS logo for two seconds on startup

    load_led(ledmat_off);
}

void set_params() {
    String msg;
    JsonDocument doc = create_data_doc("set_params", 0);
    JsonObject data = doc["Data"].to<JsonObject>();

    bool ok = handle_wifi(&doc);

    if (ok) {
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

    data["DeviceSN"] = SWPS_SN;

    load_led(LEDMATRIX_CHIP);
    
    data["Temperature"] = bme.readTemperature();
    delayMicroseconds(10);
    data["Humidity"] = bme.readHumidity();
    delayMicroseconds(10);
    data["Pressure"] = bme.readPressure() / 100.0F;
    delayMicroseconds(10);

    int16_t adc0 = ads.readADC_SingleEnded(0); // Light intensity
    delayMicroseconds(10);
    int16_t adc1 = ads.readADC_SingleEnded(1); // Water level
    delayMicroseconds(10);
    int16_t adc2 = ads.readADC_SingleEnded(2); // Soil moisture
    delayMicroseconds(10);
    int16_t adc3 = ads.readADC_SingleEnded(3); // None
    delayMicroseconds(10);
    
    data["RawValue0"] = adc0;
    data["RawValue1"] = adc1;
    data["RawValue2"] = adc2;
    data["RawValue3"] = adc3;
    data["Voltage0"] = ads.computeVolts(adc0);
    data["Voltage1"] = ads.computeVolts(adc1);
    data["Voltage2"] = ads.computeVolts(adc2);
    data["Voltage3"] = ads.computeVolts(adc3);

    load_led(ledmat_off);

    RTC.getTime(detect_time);
    double time_stamp = detect_time.getUnixTime();
    data["DetectTime"] = time_stamp;

    data["PumpStartTime"] = start_water_pump(adc2);

    bool ok = handle_wifi(&doc);

    if (ok){
        if (doc["Result"] == 1) {
            return true;
        }
        else {
            return false;
        }
    }
    else {
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

        delay(2000); // Avoid motor interference
    }
    else {
        pump_runtime = 0;
    }

    return pump_runtime;
}