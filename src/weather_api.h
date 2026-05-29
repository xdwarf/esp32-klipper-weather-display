#ifndef WEATHER_API_H
#define WEATHER_API_H

#include <HTTPClient.h>
#include <ArduinoJson.h>

struct WeatherData {
    float current_temp = 0;
    const char* current_icon = "?";
    char forecast_6h[50] = "";
    bool valid = false;
};

class WeatherAPI {
private:
    HTTPClient http;
    
    // City coordinates database (Danish cities)
    const struct CityCoord {
        const char* name;
        float lat;
        float lon;
    } cities[20] = {
        {"Copenhagen", 55.6761, 12.5683},
        {"Aarhus", 56.1567, 10.2107},
        {"Odense", 55.3950, 10.3823},
        {"Aalborg", 57.0480, 9.9191},
        {"Randers", 56.4789, 10.0294},
        {"Esbjerg", 55.4670, 8.4520},
        {"Kolding", 55.4915, 9.4846},
        {"Horsens", 55.8617, 9.8540},
        {"Silkeborg", 56.1811, 9.5525},
        {"Viborg", 55.9846, 10.4313},
        {"Svendborg", 55.0704, 10.6018},
        {"Vejle", 55.7077, 9.5360},
        {"Helsingør", 56.0358, 12.6120},
        {"Frederiksberg", 55.7186, 12.5450},
        {"Roskilde", 55.6415, 12.0752},
        {"Hillerød", 56.0343, 12.3035},
        {"Holstebro", 56.3606, 8.6197},
        {"Slagelse", 55.3327, 11.3750},
        {"Aabenraa", 54.9053, 9.4156},
        {"Tønder", 54.9439, 8.8639}
    };

    const struct CityCoord* findCity(const char* city_name) {
        for (int i = 0; i < 20; i++) {
            if (strcasecmp(cities[i].name, city_name) == 0) {
                return &cities[i];
            }
        }
        // Default to Copenhagen if not found
        return &cities[0];
    }

    const char* getWeatherIcon(const char* symbol_code) {
        // Parse YR.no symbol codes and return simple icons
        if (strstr(symbol_code, "sun") != NULL || strstr(symbol_code, "clear_sky") != NULL) {
            return "☀️";
        } else if (strstr(symbol_code, "cloud") != NULL && strstr(symbol_code, "rain") == NULL) {
            return "☁️";
        } else if (strstr(symbol_code, "rain") != NULL) {
            return "🌧️";
        } else if (strstr(symbol_code, "sleet") != NULL) {
            return "🌨️";
        } else if (strstr(symbol_code, "snow") != NULL) {
            return "❄️";
        } else if (strstr(symbol_code, "fog") != NULL) {
            return "🌫️";
        } else if (strstr(symbol_code, "thunder") != NULL) {
            return "⛈️";
        }
        return "⛅";  // Default: partly cloudy
    }

public:
    WeatherData getWeather(const char* city_name) {
        WeatherData data;

        if (!city_name || strlen(city_name) == 0) {
            Serial.println("City name not configured");
            return data;
        }

        const CityCoord* city = findCity(city_name);
        Serial.printf("Getting weather for %s (%.2f, %.2f)\n", city->name, city->lat, city->lon);

        char url[256];
        snprintf(url, sizeof(url), 
                 "https://api.met.no/weatherapi/locationforecast/2.0/complete?lat=%.2f&lon=%.2f",
                 city->lat, city->lon);

        http.begin(url);
        http.addHeader("User-Agent", "ESP32-Klipper-Display/1.0");

        int httpCode = http.GET();
        
        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            
            StaticJsonDocument<4096> doc;
            DeserializationError error = deserializeJson(doc, payload);
            
            if (!error) {
                // Get current conditions (first timeseries entry)
                JsonArray timeseries = doc["properties"]["timeseries"];
                
                if (timeseries.size() > 0) {
                    JsonObject current = timeseries[0]["data"]["instant"]["details"];
                    data.current_temp = current["air_temperature"];
                    
                    // Get weather symbol for current time
                    const char* symbol = timeseries[0]["data"]["next_1_hours"]["summary"]["symbol_code"];
                    data.current_icon = getWeatherIcon(symbol);
                    
                    // Build 6-hour forecast string from next entries
                    strcpy(data.forecast_6h, "");
                    int hour_count = 0;
                    for (int i = 1; i < timeseries.size() && hour_count < 6; i++) {
                        // Get every hourly entry for 6 hours
                        if (timeseries[i]["data"].containsKey("next_1_hours")) {
                            const char* symbol = timeseries[i]["data"]["next_1_hours"]["summary"]["symbol_code"];
                            const char* icon = getWeatherIcon(symbol);
                            
                            // Append to forecast string
                            char forecast_part[10];
                            snprintf(forecast_part, sizeof(forecast_part), "%s ", icon);
                            strlcat(data.forecast_6h, forecast_part, sizeof(data.forecast_6h));
                            
                            hour_count++;
                        }
                    }
                    
                    data.valid = true;
                    Serial.printf("Weather: %.1f°C, Icon: %s, Forecast: %s\n", 
                                  data.current_temp, data.current_icon, data.forecast_6h);
                } else {
                    Serial.println("No timeseries data found");
                }
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
        http.begin("https://api.met.no/weatherapi/locationforecast/2.0/complete?lat=55.6761&lon=12.5683");
        http.addHeader("User-Agent", "ESP32-Klipper-Display/1.0");
        int httpCode = http.GET();
        http.end();

        if (httpCode == HTTP_CODE_OK) {
            Serial.println("Weather API connection successful");
            return true;
        } else {
            Serial.printf("Weather API connection failed: %d\n", httpCode);
            return false;
        }
    }
};

#endif // WEATHER_API_H
