# ESP32 C3 Klipper + Weather Display

A web-configurable firmware for ESP32 C3 Supermini that displays:
- **Left side**: Current weather icon + 6-hour forecast
- **Right side**: 3D printer progress (layer info + percentage done)

Features:
- 📱 Web configuration interface (no code re-upload needed)
- 🖥️ SSD1306 OLED display support
- 🌐 Klipper printer integration (REST API)
- 🌦️ Weather from YR.no (Norwegian weather service - works for Denmark too!)
- ⚙️ GPIO pin configuration (customizable without firmware reupload)
- 🔘 Touch button support
- 💾 Persistent storage using SPIFFS

## Hardware Requirements

- ESP32 C3 Supermini
- SSD1306 OLED Display (128x64)
- Touch button
- I2C wiring (SDA/SCL to display)
- USB cable for power & programming

## Quick Start

### 1. Install PlatformIO
- Install VS Code: https://code.visualstudio.com/
- Install PlatformIO extension in VS Code
- Restart VS Code

### 2. Clone & Open Project
```bash
git clone https://github.com/xdwarf/esp32-klipper-weather-display.git
cd esp32-klipper-weather-display
```
Open folder in VS Code.

### 3. Build & Upload
- Connect ESP32 C3 Supermini via USB
- Click **PlatformIO → Upload** (left sidebar)
- Wait for upload to complete

### 4. Configure via Web Interface

**First boot:**
1. ESP32 creates a WiFi hotspot: `Klipper-Display-XXXXX` (password: `12345678`)
2. Connect to it from your phone/computer
3. Open browser to `192.168.4.1`
4. Fill in configuration:
   - **Printer IP**: Your Klipper instance IP (e.g., `192.168.1.100`)
   - **API Key**: Get from Klipper `moonraker.conf`
   - **City Name**: Your Danish city (e.g., "Copenhagen", "Aarhus", "Odense")
   - **GPIO Pins**: SDA, SCL, Touch Button (see defaults below)
5. Click **Save & Reboot**

**Next boots:**
- ESP32 connects to your home WiFi automatically
- Display updates every 10 seconds with printer progress & weather
- Web interface still available at `<esp32-local-ip>:80`

## Pin Configuration

Default pins (all customizable via web interface):
- **SDA (I2C Data)**: GPIO 3
- **SCL (I2C Clock)**: GPIO 2
- **Touch Button**: GPIO 5
- **Button GND**: GPIO 6 (set as OUTPUT LOW for button ground)

If you change pins, just update them in web interface and reboot - **no firmware upload needed!**

## Getting Required Info

### Klipper API Key
1. SSH into your printer: `ssh pi@<printer-ip>`
2. Edit: `~/klipper_config/moonraker.conf`
3. Find the `[authorization]` section
4. Copy the API key value
5. Paste into web interface

### City Name for Weather
- Just use your Danish city name: Copenhagen, Aarhus, Odense, Aalborg, etc.
- YR.no will automatically find coordinates and fetch weather

### Printer IP
- Usually something like `192.168.1.100` or `192.168.1.X`
- Find it via: `hostname -I` on printer terminal

## Display Layout

```
┌──────────────────────────────────┐
│ 🌡️ 5°C    │    Layer: 5 of 120   │
│ ⛅ 3 h    │    Progress: 45%     │
│ 🌧️ 6 h    │                      │
│ ⛅ next    │                      │
└──────────────────────────────────┘
```

- **Left**: Current temp + weather icons for next 6 hours
- **Right**: Layer counter + percentage done
- Updates every 10 seconds

## Touch Button Functions

- **Single Press**: Refresh printer data immediately
- **Long Press** (2+ sec): Refresh weather data immediately

## API Endpoints Used

### Klipper (Moonraker)
- `GET /printer/objects/query?objects=gcode_move,virtual_sdcard,print_stats`
- Returns: layer info, print progress, current coordinates

### YR.no Weather
- `GET https://api.met.no/weatherapi/locationforecast/2.0/complete?lat=X&lon=Y`
- Public API - no authentication needed!

## File Structure
```
├── platformio.ini          # Build configuration
├── README.md               # This file
├── src/
│   ├── main.cpp           # Main firmware
│   ├── klipper_api.h      # Klipper REST API client
│   ├── weather_api.h      # YR.no weather API client
│   ├── config.h           # Configuration management (SPIFFS)
│   ├── display.h          # SSD1306 display controller (U8g2lib)
│   └── web_server.h       # Web configuration interface
└── data/
    └── (web assets go here if needed)
```

## Supported Danish Cities

- Copenhagen, Aarhus, Odense, Aalborg, Randers, Esbjerg, Kolding, Horsens
- Silkeborg, Viborg, Svendborg, Vejle, Helsingør, Frederiksberg, Roskilde
- Hillerød, Holstebro, Slagelse, Aabenraa, Tønder
- (Other cities default to Copenhagen coordinates)

## Troubleshooting

| Issue | Solution |
|-------|----------|
| **ESP32 won't upload** | Hold BOOT button while uploading. Check COM port in PlatformIO. |
| **WiFi hotspot not appearing** | Ensure ESP32 has power. Check USB cable connection. |
| **Display not showing** | Verify SDA/SCL pins match your wiring. Check I2C address (usually 0x3C). |
| **Weather not updating** | Check if connected to WiFi. Verify city name is valid Danish city. |
| **Printer data not showing** | Verify Printer IP + API key in web interface. Check Klipper is running. |
| **Config keeps resetting** | SPIFFS might be full. Try erasing flash: `PlatformIO → Devices → Erase Flash`. |

## Performance & Updates

- **Weather**: Updates every 30 minutes (YR.no rate limit friendly)
- **Printer**: Updates every 10 seconds
- **Web interface**: Refreshes configuration instantly (after reboot)
- **Memory**: ~120KB flash used, ~80KB RAM available for operations

## License
MIT - Feel free to use, modify, and share!

## Support & Contributing

Found a bug or have a feature request? Open an issue!

Want to contribute? Submit a pull request - all improvements welcome!

---

**Built for ESP32 C3 Supermini | YR.no Weather | Klipper 3D Printer**
