#ifndef KLIPPER_API_H
#define KLIPPER_API_H

#include <HTTPClient.h>
#include <ArduinoJson.h>

struct PrinterData {
    bool is_printing = false;
    int current_layer = 0;
    int total_layers = 0;
    float progress = 0.0f;
    float x = 0, y = 0, z = 0;
    float nozzle_temp = 0, bed_temp = 0;
    char filename[64] = "";
};

class KlipperAPI {
private:
    const char* printer_ip;
    const char* api_key;
    HTTPClient http;

public:
    KlipperAPI(const char* ip, const char* key) : printer_ip(ip), api_key(key) {}

    PrinterData getPrinterStatus() {
        PrinterData data;

        if (!printer_ip || strlen(printer_ip) == 0 || !api_key || strlen(api_key) == 0) {
            Serial.println("Printer IP or API key not configured");
            return data;
        }

        char url[256];
        snprintf(url, sizeof(url), 
                 "http://%s/printer/objects/query?objects=gcode_move,virtual_sdcard,print_stats,heaters,extruder,heater_bed",
                 printer_ip);

        http.begin(url);
        http.addHeader("X-Access-Token", api_key);
        http.addHeader("Content-Type", "application/json");

        int httpCode = http.GET();
        
        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            
            StaticJsonDocument<2048> doc;
            DeserializationError error = deserializeJson(doc, payload);
            
            if (!error) {
                // Parse print stats
                JsonObject status = doc["result"]["status"];
                
                // Get print state
                if (status["print_stats"].containsKey("state")) {
                    const char* state = status["print_stats"]["state"];
                    data.is_printing = (strcmp(state, "printing") == 0);
                }

                // Get progress
                if (status["print_stats"].containsKey("info")) {
                    const char* info = status["print_stats"]["info"];
                    // Parse "X of Y" format if available
                    if (sscanf(info, "%d of %d", &data.current_layer, &data.total_layers) == 2) {
                        // Successfully parsed
                    }
                }

                // Get progress percentage
                if (status["virtual_sdcard"].containsKey("progress")) {
                    data.progress = status["virtual_sdcard"]["progress"] * 100.0f;
                }

                // Get coordinates
                if (status["gcode_move"].containsKey("position")) {
                    JsonArray pos = status["gcode_move"]["position"];
                    if (pos.size() >= 3) {
                        data.x = pos[0];
                        data.y = pos[1];
                        data.z = pos[2];
                    }
                }

                // Get temperatures
                if (status["extruder"].containsKey("temperature")) {
                    data.nozzle_temp = status["extruder"]["temperature"];
                }
                if (status["heater_bed"].containsKey("temperature")) {
                    data.bed_temp = status["heater_bed"]["temperature"];
                }

                // Get filename
                if (status["print_stats"].containsKey("filename")) {
                    const char* filename = status["print_stats"]["filename"];
                    strlcpy(data.filename, filename, sizeof(data.filename));
                }

                Serial.printf("Printer Status: Layer %d/%d, Progress: %.1f%%\n", 
                              data.current_layer, data.total_layers, data.progress);
            } else {
                Serial.printf("JSON parse error: %s\n", error.c_str());
            }
        } else {
            Serial.printf("HTTP Error: %d\n", httpCode);
        }

        http.end();
        return data;
    }

    bool testConnection() {
        if (!printer_ip || strlen(printer_ip) == 0) {
            Serial.println("Printer IP not configured");
            return false;
        }

        char url[256];
        snprintf(url, sizeof(url), "http://%s/server/info", printer_ip);

        http.begin(url);
        int httpCode = http.GET();
        http.end();

        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_UNAUTHORIZED) {
            Serial.println("Printer connection successful");
            return true;
        } else {
            Serial.printf("Printer connection failed: %d\n", httpCode);
            return false;
        }
    }
};

#endif // KLIPPER_API_H
