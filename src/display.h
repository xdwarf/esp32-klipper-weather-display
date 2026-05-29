#ifndef DISPLAY_H
#define DISPLAY_H

#include <U8g2lib.h>
#include <Wire.h>

class DisplayController {
private:
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C* u8g2;
    int sda_pin;
    int scl_pin;

public:
    DisplayController(int sda, int scl) : sda_pin(sda), scl_pin(scl) {
        // Initialize I2C with custom pins
        Wire.begin(sda_pin, scl_pin);
        
        // Create display object
        u8g2 = new U8G2_SSD1306_128X64_NONAME_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE);
        u8g2->begin();
        u8g2->setFont(u8g2_font_6x10_tf);
        u8g2->clearBuffer();
        u8g2->drawStr(10, 32, "Initializing...");
        u8g2->sendBuffer();
    }

    ~DisplayController() {
        if (u8g2) delete u8g2;
    }

    void displayStartup() {
        u8g2->clearBuffer();
        u8g2->setFont(u8g2_font_ncenB14_tr);
        u8g2->drawStr(15, 30, "Klipper");
        u8g2->drawStr(20, 48, "Display");
        u8g2->setFont(u8g2_font_6x10_tf);
        u8g2->drawStr(30, 60, "Starting...");
        u8g2->sendBuffer();
    }

    void displayAPMode(const char* ssid) {
        u8g2->clearBuffer();
        u8g2->setFont(u8g2_font_6x10_tf);
        u8g2->drawStr(5, 10, "WiFi Setup Mode");
        u8g2->drawStr(5, 22, "SSID:");
        u8g2->drawStr(35, 22, ssid);
        u8g2->drawStr(5, 34, "IP: 192.168.4.1");
        u8g2->drawStr(5, 46, "Pass: 12345678");
        u8g2->drawStr(5, 58, "Open browser");
        u8g2->sendBuffer();
    }

    void displayConnecting(const char* ssid) {
        u8g2->clearBuffer();
        u8g2->setFont(u8g2_font_6x10_tf);
        u8g2->drawStr(5, 15, "Connecting to:");
        u8g2->drawStr(5, 27, ssid);
        u8g2->drawStr(5, 42, ".");
        u8g2->sendBuffer();
    }

    void displayConnected(const char* ip) {
        u8g2->clearBuffer();
        u8g2->setFont(u8g2_font_6x10_tf);
        u8g2->drawStr(5, 15, "Connected!");
        u8g2->drawStr(5, 27, "IP:");
        u8g2->drawStr(25, 27, ip);
        u8g2->drawStr(5, 42, "Fetching data...");
        u8g2->sendBuffer();
    }

    void displayError(const char* error_msg) {
        u8g2->clearBuffer();
        u8g2->setFont(u8g2_font_6x10_tf);
        u8g2->drawStr(5, 15, "ERROR");
        u8g2->drawStr(5, 30, error_msg);
        u8g2->sendBuffer();
    }

    void displayPrinterAndWeather(
        const char* layer_info,      // "5 of 120"
        int progress_percent,         // 0-100
        float current_temp,           // 5.5
        const char* weather_icon,     // "☀️" or "⛅" etc
        const char* forecast_6h,      // "⛅ 🌧️ ☀️"
        bool wifi_connected,
        bool printer_connected
    ) {
        u8g2->clearBuffer();
        u8g2->setFont(u8g2_font_6x10_tf);

        // LEFT SIDE: Weather
        // Temperature
        char temp_str[20];
        snprintf(temp_str, sizeof(temp_str), "%.1f°C", current_temp);
        u8g2->drawStr(5, 12, temp_str);

        // Current weather icon
        u8g2->drawStr(45, 12, weather_icon);

        // 6-hour forecast
        u8g2->drawStr(5, 25, "Next 6h:");
        u8g2->drawStr(5, 35, forecast_6h);

        // Status icons (WiFi & Printer)
        if (wifi_connected) {
            u8g2->drawStr(110, 12, "W");  // WiFi indicator
        } else {
            u8g2->drawStr(110, 12, "w");  // Disconnected WiFi
        }

        if (printer_connected) {
            u8g2->drawStr(118, 12, "P");  // Printer indicator
        } else {
            u8g2->drawStr(118, 12, "p");  // Disconnected printer
        }

        // RIGHT SIDE: Printer Progress
        // Layer info
        u8g2->drawStr(68, 25, "Layer:");
        u8g2->drawStr(100, 25, layer_info);

        // Progress bar
        u8g2->drawFrame(68, 35, 59, 12);  // Border
        int bar_width = (progress_percent * 55) / 100;  // 55 pixels max width
        if (bar_width > 0) {
            u8g2->drawBox(70, 37, bar_width, 8);  // Filled bar
        }

        // Progress percentage
        char prog_str[10];
        snprintf(prog_str, sizeof(prog_str), "%d%%", progress_percent);
        u8g2->drawStr(100, 50, prog_str);

        // Bottom status line
        u8g2->setFont(u8g2_font_5x7_tf);
        u8g2->drawStr(5, 64, "Last: ");
        u8g2->drawStr(30, 64, "10s");

        u8g2->sendBuffer();
    }

    void displayNoData() {
        u8g2->clearBuffer();
        u8g2->setFont(u8g2_font_6x10_tf);
        u8g2->drawStr(10, 20, "Waiting for");
        u8g2->drawStr(15, 32, "printer data...");
        u8g2->drawStr(15, 50, "Check config");
        u8g2->sendBuffer();
    }

    void displayWebConfig() {
        u8g2->clearBuffer();
        u8g2->setFont(u8g2_font_6x10_tf);
        u8g2->drawStr(10, 15, "Web Config:");
        u8g2->drawStr(10, 28, "192.168.4.1");
        u8g2->drawStr(10, 42, "Configure all");
        u8g2->drawStr(10, 56, "settings here");
        u8g2->sendBuffer();
    }

    void clear() {
        u8g2->clearBuffer();
        u8g2->sendBuffer();
    }

    void test() {
        u8g2->clearBuffer();
        u8g2->setFont(u8g2_font_6x10_tf);
        u8g2->drawStr(5, 10, "Display Test OK");
        u8g2->sendBuffer();
    }
};

#endif // DISPLAY_H
