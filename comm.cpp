#include "comm.h"

static AES128 aes128;
static WiFiClient client;
static int status = WL_IDLE_STATUS;

void setup_comm() {
    uint8_t wifi_retry_times = 0;
    bool wifi_check = true;

    Serial.begin(115200);
    aes128.setKey(AES_KEY, 16);

    WiFiCfg wifi_cfg = read_wificfg_eeprom(0);

    // check for the WiFi module:
    if (WiFi.status() == WL_NO_MODULE) {
        load_led(LEDMATRIX_DANGER);
        // don't continue
        while (true);
    }

    // check or set the WiFi config:
    if (String((char *)wifi_cfg.ssid) == "") {
        wifi_check = false;
    }

    // attempt to connect to WiFi network:
    while ((status != WL_CONNECTED) && (wifi_check)) {
        load_led(LEDMATRIX_CLOUD_WIFI);
        // Connect to WPA/WPA2 network.
        status = WiFi.begin((char *)wifi_cfg.ssid, (char *)wifi_cfg.password);
     
        // wait 10 seconds for connection:
        delay(10000);

        if (wifi_retry_times > 3) {
            load_led(LEDMATRIX_EMOJI_SAD);
            // don't continue
            wifi_check = false;
        }

        wifi_retry_times++;
    }

    wifi_retry_times = 0;

    // attempt to connect to SWPS IoT server
    while ((!client.connected()) && (wifi_check)) {
        load_led(swps_logo);

        client.connect(wifi_cfg.serverip, wifi_cfg.serverport);

        // wait 10 seconds for connection:
        delay(10000);

        if (wifi_retry_times > 3) {
            load_led(LEDMATRIX_EMOJI_SAD);
            // don't continue
            wifi_check = false;
        }

        wifi_retry_times++;
    }

    while (!wifi_check) {
        handle_serial();
    }

    load_led(ledmat_off);
}

void handle_serial() {
    JsonDocument doc;
    String msg;

    if (Serial.available() > 0) {
        msg = Serial.readStringUntil('\n');
        DeserializationError err = deserializeJson(doc, msg);

        if (err.code() != DeserializationError::Ok) {
            delay(100);
        }
        else if (doc["Api"] == "get_device_sn") {
            doc = create_data_doc("", 1);
            JsonObject data = doc["Data"].to<JsonObject>();
            data["DeviceSN"] = SWPS_SN;
            serializeJson(doc, msg);
            Serial.println(msg);
        }
        else if (doc["Api"] == "reset_wifi") {
            if (doc["Data"]["DeviceSN"] == SWPS_SN) {
                String ssid = String(doc["Data"]["Ssid"]);
                String pwd = String(doc["Data"]["Password"]);
                String server_ip = String(doc["Data"]["ServerIP"]);
                uint16_t server_port = doc["Data"]["ServerPort"];
                reset_wifi_cfg(ssid, pwd, server_ip, server_port);
            }
        }
    }
}

bool handle_wifi(JsonDocument *doc) {
    if (!client.connected()) {
        reconnect_wifi_client();
    }

    String msg;

    load_led(LEDMATRIX_CLOUD_WIFI);

    serializeJson(*doc, msg);
    client.print(msg);

    while (!client.available()) {};
    msg = client.readStringUntil('\n');
    DeserializationError err = deserializeJson(*doc, msg);

    load_led(ledmat_off);

    if (err.code() == DeserializationError::Ok) {
        return true;
    }
    else {
        return false;
    }
}

void reset_wifi_cfg(String ssid, String pwd, String server_ip, uint16_t server_port) {
    digitalWrite(LED_BUILTIN, HIGH);

    write_wificfg_eeprom(0, ssid, pwd, server_ip, server_port);

    WiFiCfg wifi_cfg = read_wificfg_eeprom(0);

    digitalWrite(LED_BUILTIN, LOW);
    load_led(LEDMATRIX_EMOJI_HAPPY);

    delay(2000);

    NVIC_SystemReset();
    while (true);
}

void reconnect_wifi_client() {
    client.stop();
    load_led(LEDMATRIX_CLOUD_WIFI);

    digitalWrite(LED_BUILTIN, HIGH);
    WiFiCfg wifi_cfg = read_wificfg_eeprom(0);
    digitalWrite(LED_BUILTIN, LOW);

    // wait 10 seconds for connection:
    delay(10000);

    if (WiFi.status() != WL_CONNECTED) {
        load_led(LEDMATRIX_EMOJI_SAD);
        while (true);
    }

    client.connect(wifi_cfg.serverip, wifi_cfg.serverport);

    load_led(ledmat_off);
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