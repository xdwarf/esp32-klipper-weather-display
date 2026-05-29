#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include "config.h"

class WebServer {
private:
    AsyncWebServer server;
    ConfigManager& config_manager;
    bool ap_mode;

public:
    WebServer(ConfigManager& cm, int port = 80) 
        : server(port), config_manager(cm), ap_mode(false) {}

    void startAPMode(const char* ssid, const char* password) {
        ap_mode = true;
        WiFi.mode(WIFI_AP);
        WiFi.softAP(ssid, password);
        Serial.printf("AP Mode: SSID=%s, IP=%s\n", ssid, WiFi.softAPIP().toString().c_str());
        setupRoutes();
        server.begin();
        Serial.println("Web server started (AP mode)");
    }

    void startStationMode() {
        ap_mode = false;
        setupRoutes();
        server.begin();
        Serial.println("Web server started (Station mode)");
    }

    void stop() {
        server.end();
    }

private:
    void setupRoutes() {
        // Serve index.html
        server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
            if (SPIFFS.exists("/index.html")) {
                request->send(SPIFFS, "/index.html", "text/html");
            } else {
                request->send(200, "text/html", getEmbeddedHTML());
            }
        });

        // Get current config
        server.on("/api/config", HTTP_GET, [this](AsyncWebServerRequest *request) {
            Config& cfg = config_manager.getConfig();
            
            StaticJsonDocument<512> doc;
            doc["printer_ip"] = cfg.printer_ip;
            doc["api_key"] = cfg.api_key;
            doc["city_name"] = cfg.city_name;
            doc["ssid"] = cfg.ssid;
            doc["password"] = cfg.password;
            doc["sda_pin"] = cfg.sda_pin;
            doc["scl_pin"] = cfg.scl_pin;
            doc["button_pin"] = cfg.button_pin;
            doc["button_gnd_pin"] = cfg.button_gnd_pin;

            String json;
            serializeJson(doc, json);
            request->send(200, "application/json", json);
        });

        // Save config
        server.on("/api/config", HTTP_POST, [this](AsyncWebServerRequest *request) {
            // Empty POST handler, actual data in body
        }, NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            StaticJsonDocument<512> doc;
            DeserializationError error = deserializeJson(doc, (const char*)data);

            if (!error) {
                config_manager.updatePrinterIP(doc["printer_ip"] | "");
                config_manager.updateAPIKey(doc["api_key"] | "");
                config_manager.updateCityName(doc["city_name"] | "");
                config_manager.updateSSID(doc["ssid"] | "");
                config_manager.updatePassword(doc["password"] | "");
                config_manager.updateSDAPIn(doc["sda_pin"] | DEFAULT_SDA);
                config_manager.updateSCLPin(doc["scl_pin"] | DEFAULT_SCL);
                config_manager.updateButtonPin(doc["button_pin"] | DEFAULT_BUTTON_PIN);
                config_manager.updateButtonGNDPin(doc["button_gnd_pin"] | DEFAULT_BUTTON_GND);
                
                config_manager.saveConfig();
                request->send(200, "application/json", "{\"status\":\"ok\"}");
            } else {
                request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            }
        });

        // Trigger reboot
        server.on("/api/reboot", HTTP_POST, [this](AsyncWebServerRequest *request) {
            request->send(200, "application/json", "{\"status\":\"rebooting\"}");
            delay(1000);
            ESP.restart();
        });

        // Catch-all for SPA
        server.onNotFound([this](AsyncWebServerRequest *request) {
            if (request->method() == HTTP_GET) {
                request->send(SPIFFS, "/index.html", "text/html");
            } else {
                request->send(404);
            }
        });
    }

    String getEmbeddedHTML() {
        return R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Klipper Display Config</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            padding: 20px;
        }
        
        .container {
            background: white;
            border-radius: 20px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            padding: 40px;
            max-width: 500px;
            width: 100%;
        }
        
        h1 {
            color: #333;
            margin-bottom: 10px;
            font-size: 28px;
            text-align: center;
        }
        
        .subtitle {
            text-align: center;
            color: #666;
            margin-bottom: 30px;
            font-size: 14px;
        }
        
        .form-group {
            margin-bottom: 25px;
        }
        
        label {
            display: block;
            margin-bottom: 8px;
            color: #333;
            font-weight: 500;
            font-size: 14px;
        }
        
        input, textarea {
            width: 100%;
            padding: 12px;
            border: 2px solid #e0e0e0;
            border-radius: 8px;
            font-size: 14px;
            transition: border-color 0.3s;
            font-family: inherit;
        }
        
        input:focus, textarea:focus {
            outline: none;
            border-color: #667eea;
        }
        
        textarea {
            resize: vertical;
            min-height: 80px;
            font-family: monospace;
        }
        
        .section-title {
            background: #f5f5f5;
            padding: 15px;
            border-radius: 8px;
            margin-top: 30px;
            margin-bottom: 15px;
            font-weight: 600;
            color: #667eea;
            font-size: 13px;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }
        
        .pin-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 15px;
        }
        
        .status {
            padding: 15px;
            border-radius: 8px;
            margin-top: 20px;
            text-align: center;
            font-size: 14px;
            display: none;
        }
        
        .status.success {
            background: #e8f5e9;
            color: #2e7d32;
            display: block;
        }
        
        .status.error {
            background: #ffebee;
            color: #c62828;
            display: block;
        }
        
        .buttons {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 10px;
            margin-top: 30px;
        }
        
        button {
            padding: 14px 20px;
            border: none;
            border-radius: 8px;
            font-weight: 600;
            cursor: pointer;
            font-size: 14px;
            transition: all 0.3s;
        }
        
        .btn-save {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            grid-column: 1 / 2;
        }
        
        .btn-save:hover {
            transform: translateY(-2px);
            box-shadow: 0 10px 20px rgba(102, 126, 234, 0.3);
        }
        
        .btn-reboot {
            background: #ff6b6b;
            color: white;
            grid-column: 2 / 3;
        }
        
        .btn-reboot:hover {
            background: #ff5252;
            transform: translateY(-2px);
        }
        
        .info-box {
            background: #e3f2fd;
            border-left: 4px solid #2196f3;
            padding: 15px;
            border-radius: 4px;
            margin-bottom: 20px;
            font-size: 13px;
            color: #1565c0;
            line-height: 1.6;
        }
        
        .loading {
            display: none;
            text-align: center;
            padding: 20px;
        }
        
        .spinner {
            border: 3px solid #f3f3f3;
            border-top: 3px solid #667eea;
            border-radius: 50%;
            width: 40px;
            height: 40px;
            animation: spin 1s linear infinite;
            margin: 0 auto;
        }
        
        @keyframes spin {
            0% { transform: rotate(0deg); }
            100% { transform: rotate(360deg); }
        }
        
        .hint {
            font-size: 12px;
            color: #999;
            margin-top: 5px;
            font-style: italic;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🖨️ Klipper Display</h1>
        <p class="subtitle">Configuration Interface</p>
        
        <div class="info-box">
            ℹ️ Configure your printer, weather, and GPIO pins. Settings are saved to device storage.
        </div>
        
        <form id="configForm">
            <!-- Printer Configuration -->
            <div class="section-title">Printer Settings</div>
            
            <div class="form-group">
                <label for="printer_ip">Printer IP Address</label>
                <input type="text" id="printer_ip" name="printer_ip" placeholder="192.168.1.100" required>
                <div class="hint">Your Klipper instance IP on local network</div>
            </div>
            
            <div class="form-group">
                <label for="api_key">Klipper API Key</label>
                <textarea id="api_key" name="api_key" placeholder="Paste your Moonraker API key here..."></textarea>
                <div class="hint">Found in ~/klipper_config/moonraker.conf</div>
            </div>
            
            <!-- Weather Configuration -->
            <div class="section-title">Weather Settings</div>
            
            <div class="form-group">
                <label for="city_name">Danish City Name</label>
                <input type="text" id="city_name" name="city_name" placeholder="Copenhagen" required>
                <div class="hint">Supported: Copenhagen, Aarhus, Odense, Aalborg, Randers, Esbjerg, etc.</div>
            </div>
            
            <!-- WiFi Configuration -->
            <div class="section-title">WiFi Settings</div>
            
            <div class="form-group">
                <label for="ssid">WiFi SSID</label>
                <input type="text" id="ssid" name="ssid" placeholder="Your WiFi network name">
                <div class="hint">Leave empty to keep using access point</div>
            </div>
            
            <div class="form-group">
                <label for="password">WiFi Password</label>
                <input type="password" id="password" name="password" placeholder="Your WiFi password">
                <div class="hint">Only used if SSID is filled</div>
            </div>
            
            <!-- GPIO Configuration -->
            <div class="section-title">GPIO Pin Configuration</div>
            
            <div class="pin-grid">
                <div class="form-group">
                    <label for="sda_pin">SDA Pin (I2C Data)</label>
                    <input type="number" id="sda_pin" name="sda_pin" min="0" max="46" value="3" required>
                </div>
                
                <div class="form-group">
                    <label for="scl_pin">SCL Pin (I2C Clock)</label>
                    <input type="number" id="scl_pin" name="scl_pin" min="0" max="46" value="2" required>
                </div>
                
                <div class="form-group">
                    <label for="button_pin">Touch Button Pin</label>
                    <input type="number" id="button_pin" name="button_pin" min="0" max="46" value="5" required>
                </div>
                
                <div class="form-group">
                    <label for="button_gnd_pin">Button GND Pin</label>
                    <input type="number" id="button_gnd_pin" name="button_gnd_pin" min="0" max="46" value="6" required>
                </div>
            </div>
            
            <div class="info-box" style="margin-top: 20px;">
                ℹ️ Change GPIO pins and reboot without reflashing firmware!
            </div>
            
            <!-- Status Messages -->
            <div id="status" class="status"></div>
            
            <div class="loading" id="loading">
                <div class="spinner"></div>
                <p>Saving configuration...</p>
            </div>
            
            <!-- Buttons -->
            <div class="buttons">
                <button type="submit" class="btn-save">💾 Save</button>
                <button type="button" class="btn-reboot" onclick="rebootDevice()">🔄 Reboot</button>
            </div>
        </form>
    </div>

    <script>
        const form = document.getElementById('configForm');
        const statusDiv = document.getElementById('status');
        const loadingDiv = document.getElementById('loading');

        // Load current config
        async function loadConfig() {
            try {
                const response = await fetch('/api/config');
                const data = await response.json();
                
                document.getElementById('printer_ip').value = data.printer_ip || '';
                document.getElementById('api_key').value = data.api_key || '';
                document.getElementById('city_name').value = data.city_name || '';
                document.getElementById('ssid').value = data.ssid || '';
                document.getElementById('password').value = data.password || '';
                document.getElementById('sda_pin').value = data.sda_pin || 3;
                document.getElementById('scl_pin').value = data.scl_pin || 2;
                document.getElementById('button_pin').value = data.button_pin || 5;
                document.getElementById('button_gnd_pin').value = data.button_gnd_pin || 6;
            } catch (error) {
                console.error('Error loading config:', error);
                showStatus('Error loading configuration', 'error');
            }
        }

        // Save config
        form.addEventListener('submit', async (e) => {
            e.preventDefault();
            
            loadingDiv.style.display = 'block';
            statusDiv.style.display = 'none';

            const formData = {
                printer_ip: document.getElementById('printer_ip').value,
                api_key: document.getElementById('api_key').value,
                city_name: document.getElementById('city_name').value,
                ssid: document.getElementById('ssid').value,
                password: document.getElementById('password').value,
                sda_pin: parseInt(document.getElementById('sda_pin').value),
                scl_pin: parseInt(document.getElementById('scl_pin').value),
                button_pin: parseInt(document.getElementById('button_pin').value),
                button_gnd_pin: parseInt(document.getElementById('button_gnd_pin').value)
            };

            try {
                const response = await fetch('/api/config', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json'
                    },
                    body: JSON.stringify(formData)
                });

                const result = await response.json();
                loadingDiv.style.display = 'none';

                if (response.ok) {
                    showStatus('✅ Configuration saved successfully! Device will use new settings on next reboot.', 'success');
                } else {
                    showStatus('❌ Error saving configuration', 'error');
                }
            } catch (error) {
                loadingDiv.style.display = 'none';
                console.error('Error saving config:', error);
                showStatus('❌ Network error while saving', 'error');
            }
        });

        // Reboot device
        async function rebootDevice() {
            if (!confirm('Are you sure you want to reboot the device?')) return;

            loadingDiv.style.display = 'block';
            
            try {
                await fetch('/api/reboot', {
                    method: 'POST'
                });
                
                showStatus('⏳ Device is rebooting... Please wait 10 seconds', 'success');
                setTimeout(() => {
                    window.location.reload();
                }, 5000);
            } catch (error) {
                loadingDiv.style.display = 'none';
                console.error('Error rebooting:', error);
                showStatus('Reboot command sent (may have worked)', 'success');
            }
        }

        function showStatus(message, type) {
            statusDiv.textContent = message;
            statusDiv.className = `status ${type}`;
            statusDiv.style.display = 'block';
        }

        // Load config on page load
        loadConfig();
    </script>
</body>
</html>
        )";
    }
};

#endif // WEB_SERVER_H
