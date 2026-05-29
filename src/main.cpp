#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include "config.h"
#include "display.h"
#include "klipper_api.h"
#include "weather_api.h"
#include "web_server.h"

// Global objects
ConfigManager config_manager;
DisplayController* display = nullptr;
KlipperAPI* klipper_api = nullptr;
WeatherAPI* weather_api = nullptr;
WebServer* web_server = nullptr;

// State variables
bool wifi_connected = false;
bool printer_connected = false;
unsigned long last_printer_update = 0;
unsigned long last_weather_update = 0;
PrinterData printer_data;
WeatherData weather_data;

// Timing constants (milliseconds)
const unsigned long PRINTER_UPDATE_INTERVAL = 10000;    // 10 seconds
const unsigned long WEATHER_UPDATE_INTERVAL = 1800000;  // 30 minutes
const unsigned long WIFI_CHECK_INTERVAL = 5000;         // 5 seconds

// Button handling
volatile bool button_pressed = false;
unsigned long button_press_time = 0;
const unsigned long LONG_PRESS_TIME = 2000;  // 2 seconds

void IRAM_ATTR buttonISR() {
    button_pressed = true;
    button_press_time = millis();
}

void initializeDisplay() {
    Config& cfg = config_manager.getConfig();
    display = new DisplayController(cfg.sda_pin, cfg.scl_pin);
    display->displayStartup();
}

void connectToWiFi() {
    Config& cfg = config_manager.getConfig();
    
    // Check if WiFi SSID is configured
    if (strlen(cfg.ssid) > 0) {
        // Connect to configured WiFi
        Serial.printf("Connecting to WiFi: %s\n", cfg.ssid);
        display->displayConnecting(cfg.ssid);
        
        WiFi.mode(WIFI_STA);
        WiFi.begin(cfg.ssid, cfg.password);
        
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            Serial.print(".");
            attempts++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("\nConnected! IP: %s\n", WiFi.localIP().toString().c_str());
            display->displayConnected(WiFi.localIP().toString().c_str());
            wifi_connected = true;
            web_server->startStationMode();
            delay(2000);
        } else {
            Serial.println("\nFailed to connect to WiFi, starting AP mode");
            wifi_connected = false;
            setupAPMode();
        }
    } else {
        // No WiFi configured, start AP mode
        setupAPMode();
    }
}

void setupAPMode() {
    Serial.println("Starting AP mode for configuration");
    char ssid[32];
    uint32_t chip_id = (uint32_t)ESP.getEfuseMac();
    snprintf(ssid, sizeof(ssid), "Klipper-Display-%06X", chip_id & 0xFFFFFF);
    
    web_server->startAPMode(ssid, "12345678");
    display->displayAPMode(ssid);
    wifi_connected = false;
}

void updatePrinterData() {
    if (!wifi_connected || !klipper_api) return;
    
    unsigned long now = millis();
    if (now - last_printer_update < PRINTER_UPDATE_INTERVAL) return;
    
    last_printer_update = now;
    
    printer_data = klipper_api->getPrinterStatus();
    printer_connected = true;
    
    Serial.printf("Printer: Layer %d/%d, Progress: %.1f%%\n",
                  printer_data.current_layer, printer_data.total_layers, printer_data.progress);
}

void updateWeatherData() {
    if (!wifi_connected || !weather_api) return;
    
    unsigned long now = millis();
    if (now - last_weather_update < WEATHER_UPDATE_INTERVAL) return;
    
    last_weather_update = now;
    
    Config& cfg = config_manager.getConfig();
    weather_data = weather_api->getWeather(cfg.city_name);
    
    if (weather_data.valid) {
        Serial.printf("Weather: %.1f°C, Icon: %s\n", weather_data.current_temp, weather_data.current_icon);
    }
}

void displayUpdateScreen() {
    if (!display) return;
    
    if (!wifi_connected) {
        display->displayWebConfig();
        return;
    }
    
    if (!printer_data.is_printing) {
        display->displayNoData();
        return;
    }
    
    // Format layer info
    char layer_str[20];
    snprintf(layer_str, sizeof(layer_str), "%d of %d",
             printer_data.current_layer, printer_data.total_layers);
    
    // Get weather data (use fallback if not available)
    const char* weather_icon = weather_data.valid ? weather_data.current_icon : "?";
    const char* forecast = weather_data.valid ? weather_data.forecast_6h : "No data";
    float temp = weather_data.valid ? weather_data.current_temp : 0;
    
    // Display everything
    display->displayPrinterAndWeather(
        layer_str,
        (int)printer_data.progress,
        temp,
        weather_icon,
        forecast,
        wifi_connected,
        printer_connected
    );
}

void handleButtonPress() {
    if (!button_pressed) return;
    
    unsigned long press_duration = millis() - button_press_time;
    
    if (press_duration > LONG_PRESS_TIME) {
        // Long press: Refresh weather immediately
        Serial.println("Long press - refreshing weather");
        last_weather_update = 0;  // Force update next iteration
    } else {
        // Short press: Refresh printer data
        Serial.println("Short press - refreshing printer data");
        last_printer_update = 0;  // Force update next iteration
    }
    
    button_pressed = false;
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n\n=== ESP32 Klipper Display ===");
    Serial.printf("CPU Frequency: %d MHz\n", getCpuFreqMHz());
    
    // Load configuration
    config_manager.loadConfig();
    Config& cfg = config_manager.getConfig();
    
    // Initialize display
    initializeDisplay();
    
    // Setup button
    pinMode(cfg.button_pin, INPUT_PULLUP);
    pinMode(cfg.button_gnd_pin, OUTPUT);
    digitalWrite(cfg.button_gnd_pin, LOW);
    attachInterrupt(digitalPinToInterrupt(cfg.button_pin), buttonISR, FALLING);
    
    // Initialize APIs and web server
    klipper_api = new KlipperAPI(cfg.printer_ip, cfg.api_key);
    weather_api = new WeatherAPI();
    web_server = new WebServer(config_manager);
    
    // Connect to WiFi
    connectToWiFi();
    
    // Sync time with NTP
    Serial.println("Syncing time with NTP...");
    configTime(3600, 0, "pool.ntp.org", "time.nist.gov");
    
    Serial.println("Setup complete!");
}

void loop() {
    // Check WiFi connection periodically
    static unsigned long last_wifi_check = 0;
    unsigned long now = millis();
    
    if (now - last_wifi_check > WIFI_CHECK_INTERVAL) {
        last_wifi_check = now;
        
        if (WiFi.status() != WL_CONNECTED && wifi_connected) {
            Serial.println("WiFi connection lost!");
            wifi_connected = false;
            setupAPMode();
        }
    }
    
    // Update printer and weather data
    updatePrinterData();
    updateWeatherData();
    
    // Handle button presses
    handleButtonPress();
    
    // Update display
    displayUpdateScreen();
    
    yield();  // Let WiFi task run
}
