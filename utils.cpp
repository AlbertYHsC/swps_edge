#include "Arduino_LED_Matrix.h"
#include "utils.h"

ArduinoLEDMatrix matrix;

const uint32_t swps_logo[3] = {
    0x77384967,
    0x147f581,
    0x56951cae
};

const uint32_t ledmat_off[3] = {
    0x0,
    0x0,
    0x0
};

void setup_led() {
    matrix.begin();
    pinMode(LED_BUILTIN, OUTPUT);
}

void load_led(const uint32_t buffer[3]) {
    matrix.loadFrame(buffer);
}

JsonDocument create_data_doc(String api, int result) {
    JsonDocument doc;
    doc["Api"] = api;
    doc["Result"] = result;

    return doc;
}