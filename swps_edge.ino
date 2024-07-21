#include <ArduinoJson.h>
#include "RTC.h"

#include "swps_secrets.h"
#include "swps.h"
#include "comm.h"
#include "utils.h"

bool detect_lock = false;

void setup() {
    bool setup_check = false;

    setup_led();
    setup_swps();
    setup_comm();

    while (!setup_check) {
        JsonDocument doc = create_data_doc("setup_edge", 1);
        JsonObject data = doc["Data"].to<JsonObject>();
        data["DeviceSN"] = SWPS_SN;

        setup_check = handle_wifi(&doc);

        if (setup_check) {
            if (doc["Result"] != 1) {
                setup_check = false;
            }
        }
    }
    
    set_params();
}

void loop() {
    RTCTime currentTime;

    RTC.getTime(currentTime);

    handle_serial();

    if ((currentTime.getMinutes() % detect_interval == 0) && (!detect_lock)) {
        set_params();
        detect_lock = upload_sensor_record();
        if (detect_lock) {
            load_led(LEDMATRIX_HEART_SMALL);
            delay(10000); // Avoid RTC bias causing secondary measurements
        }
        else {
            delay(2000); // Try measuring again after two seconds
        }
    }
    else if (currentTime.getMinutes() % detect_interval != 0) {
        detect_lock = false;
        load_led(ledmat_off);
    }
}
