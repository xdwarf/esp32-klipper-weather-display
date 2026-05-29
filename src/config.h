#ifndef CONFIG_H
#define CONFIG_H

#include <SPIFFS.h>
#include <ArduinoJson.h>

// Default pin assignments
#define DEFAULT_SDA 3
#define DEFAULT_SCL 2
#define DEFAULT_BUTTON_PIN 5
#define DEFAULT_BUTTON_GND 6

// Configuration structure
struct Config {
    char printer_ip[50];
    char api_key[256];
    char city_name[50];
    int sda_pin;
    int scl_pin;
    int button_pin;
    int button_gnd_pin;
    char ssid[32];
    char password[64];
};

class ConfigManager {
private:
    const char* CONFIG_FILE = "/config.json";
    Config config;

public:
    ConfigManager() {
        // Initialize SPIFFS
        if (!SPIFFS.begin(true)) {
            Serial.println("SPIFFS Mount Failed");
        }
        loadConfig();
    }

    void loadConfig() {
        if (SPIFFS.exists(CONFIG_FILE)) {
            File file = SPIFFS.open(CONFIG_FILE, "r");
            if (file) {
                StaticJsonDocument<512> doc;
                DeserializationError error = deserializeJson(doc, file);
                
                if (!error) {
                    strlcpy(config.printer_ip, doc["printer_ip"] | "192.168.1.100", sizeof(config.printer_ip));
                    strlcpy(config.api_key, doc["api_key"] | "", sizeof(config.api_key));
                    strlcpy(config.city_name, doc["city_name"] | "Copenhagen", sizeof(config.city_name));
                    strlcpy(config.ssid, doc["ssid"] | "", sizeof(config.ssid));
                    strlcpy(config.password, doc["password"] | "", sizeof(config.password));
                    config.sda_pin = doc["sda_pin"] | DEFAULT_SDA;
                    config.scl_pin = doc["scl_pin"] | DEFAULT_SCL;
                    config.button_pin = doc["button_pin"] | DEFAULT_BUTTON_PIN;
                    config.button_gnd_pin = doc["button_gnd_pin"] | DEFAULT_BUTTON_GND;
                    
                    Serial.println("Config loaded from SPIFFS");
                } else {
                    Serial.println("JSON parse error, using defaults");
                    setDefaults();
                }
                file.close();
            }
        } else {
            Serial.println("Config file not found, using defaults");
            setDefaults();
        }
    }

    void saveConfig() {
        StaticJsonDocument<512> doc;
        doc["printer_ip"] = config.printer_ip;
        doc["api_key"] = config.api_key;
        doc["city_name"] = config.city_name;
        doc["ssid"] = config.ssid;
        doc["password"] = config.password;
        doc["sda_pin"] = config.sda_pin;
        doc["scl_pin"] = config.scl_pin;
        doc["button_pin"] = config.button_pin;
        doc["button_gnd_pin"] = config.button_gnd_pin;

        File file = SPIFFS.open(CONFIG_FILE, "w");
        if (file) {
            serializeJson(doc, file);
            file.close();
            Serial.println("Config saved to SPIFFS");
        } else {
            Serial.println("Failed to save config");
        }
    }

    void setDefaults() {
        strlcpy(config.printer_ip, "192.168.1.100", sizeof(config.printer_ip));
        strlcpy(config.api_key, "", sizeof(config.api_key));
        strlcpy(config.city_name, "Copenhagen", sizeof(config.city_name));
        strlcpy(config.ssid, "", sizeof(config.ssid));
        strlcpy(config.password, "", sizeof(config.password));
        config.sda_pin = DEFAULT_SDA;
        config.scl_pin = DEFAULT_SCL;
        config.button_pin = DEFAULT_BUTTON_PIN;
        config.button_gnd_pin = DEFAULT_BUTTON_GND;
    }

    Config& getConfig() { return config; }

    void updatePrinterIP(const char* ip) { strlcpy(config.printer_ip, ip, sizeof(config.printer_ip)); }
    void updateAPIKey(const char* key) { strlcpy(config.api_key, key, sizeof(config.api_key)); }
    void updateCityName(const char* city) { strlcpy(config.city_name, city, sizeof(config.city_name)); }
    void updateSSID(const char* ssid) { strlcpy(config.ssid, ssid, sizeof(config.ssid)); }
    void updatePassword(const char* pwd) { strlcpy(config.password, pwd, sizeof(config.password)); }
    void updateSDAPIn(int pin) { config.sda_pin = pin; }
    void updateSCLPin(int pin) { config.scl_pin = pin; }
    void updateButtonPin(int pin) { config.button_pin = pin; }
    void updateButtonGNDPin(int pin) { config.button_gnd_pin = pin; }
};

#endif // CONFIG_H
