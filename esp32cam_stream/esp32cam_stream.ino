/*
 * AgroBot ESP32-CAM Module
 * Real-time Video Streaming with Power Management
 * 
 * This module provides live video feed for the AgroBot
 * Compatible with both WiFi and Hotspot modes
 * Includes brownout protection and power optimization
 */

#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClient.h>
#include "soc/soc.h"           // Disable brownout problems
#include "soc/rtc_cntl_reg.h"  // Disable brownout problems
#include "esp_system.h"
#include <ESPmDNS.h>

// Camera model - AI Thinker ESP32-CAM
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

// Network Configuration
const char* wifi_ssid = "Compiler Error";     // Replace with your WiFi SSID
const char* wifi_password = "luffy@@12345"; // Replace with your WiFi password
const char* ap_ssid = "AgroBot_Hotspot";      // Must match main ESP32
const char* ap_password = "agrobot123";       // Must match main ESP32
const char* softap_ssid = "AgroBot_Camera";   // Camera's own hotspot SSID (fallback)
const char* softap_password = "agrobot123";   // Camera's own hotspot password

// Web Server
WebServer server(81); // Different port from main ESP32

// Camera Configuration
camera_config_t config;
bool camera_initialized = false;

// Status LED with power management
#define LED_PIN 4
#define FLASH_LED_PIN 4
bool led_enabled = true;
unsigned long last_led_blink = 0;

// WiFi event logging for diagnostics
void WiFiEvent(WiFiEvent_t event) {
  Serial.printf("[WiFi-event] event: %d\n", event);
}

void setup() {
  // Disable brownout detector to prevent resets during power spikes
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  
  // Reduce CPU frequency to save power
  setCpuFrequencyMhz(160); // Down from default 240MHz
  
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println("ESP32-CAM Starting with Power Management...");

  // Register WiFi event handler
  WiFi.onEvent(WiFiEvent);
  
  // Initialize LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, LOW);
  
  // Power optimization delay
  delay(1000);
  
  // Initialize camera with power-optimized settings
  if (initCamera()) {
    Serial.println("Camera initialized successfully");
    camera_initialized = true;
    // Brief LED flash to indicate camera ready (reduce power usage)
    for(int i = 0; i < 2; i++) {
      digitalWrite(LED_PIN, HIGH);
      delay(100);
      digitalWrite(LED_PIN, LOW);
      delay(100);
    }
  } else {
    Serial.println("Camera initialization failed!");
    // Slow blink to indicate error (save power)
    for(int i = 0; i < 5; i++) {
      digitalWrite(LED_PIN, HIGH);
      delay(200);
      digitalWrite(LED_PIN, LOW);
      delay(800);
    }
  }
  
  // Connect to network
  connectToNetwork();
  
  // Setup web server routes
  setupServer();
  
  // Start server
  server.begin();
  Serial.println("ESP32-CAM Server started");
  IPAddress camIp = (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) ? WiFi.softAPIP() : WiFi.localIP();
  Serial.println("Open UI: http://" + camIp.toString() + ":81/");
  Serial.println("Stream URL: http://" + camIp.toString() + ":81/stream");
  
  // Turn off LED after initialization to save power
  digitalWrite(LED_PIN, LOW);
  Serial.println("Power management active - LED disabled to save power");
}

void loop() {
  server.handleClient();
  
  // Heartbeat LED (brief flash every 5 seconds to show it's alive)
  if (millis() - last_led_blink > 5000) {
    digitalWrite(LED_PIN, HIGH);
    delay(50);
    digitalWrite(LED_PIN, LOW);
    last_led_blink = millis();
  }
  
  delay(10); // Slightly longer delay to reduce CPU usage
}

bool initCamera() {
  // Ensure camera is powered up
  pinMode(PWDN_GPIO_NUM, OUTPUT);
  digitalWrite(PWDN_GPIO_NUM, LOW); // Power up the camera sensor
  delay(10);

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // Power-optimized settings to prevent brownouts
  if(psramFound()) {
    config.frame_size = FRAMESIZE_CIF; // 400x296 - reduced from VGA for lower power
    config.jpeg_quality = 15;          // 0-63, slightly lower quality for less processing
    config.fb_count = 1;               // Reduced buffer count to save memory and power
    config.fb_location = CAMERA_FB_IN_PSRAM;
    Serial.println("PSRAM found - using power-optimized settings");
  } else {
    config.frame_size = FRAMESIZE_QCIF; // 176x144 - very small for minimal power
    config.jpeg_quality = 20;           // Lower quality for power savings
    config.fb_count = 1;
    config.fb_location = CAMERA_FB_IN_DRAM;
    Serial.println("PSRAM not found - using minimal power settings");
  }
  config.grab_mode = CAMERA_GRAB_LATEST;

  // Attempt initialization with fallbacks
  for (int attempt = 1; attempt <= 3; attempt++) {
    Serial.print("Camera init attempt ");
    Serial.println(attempt);
    esp_err_t err = esp_camera_init(&config);
    if (err == ESP_OK) {
      Serial.println("Camera init OK");
      // Get camera sensor
      sensor_t * s = esp_camera_sensor_get();
      if (s == NULL) {
        Serial.println("Failed to get camera sensor");
        return false;
      }

      // Power-optimized camera settings
      s->set_brightness(s, 0);     // -2 to 2
      s->set_contrast(s, 0);       // -2 to 2
      s->set_saturation(s, -1);    // Slightly reduced saturation to save processing
      s->set_special_effect(s, 0); // 0 to 6 (0 - No Effect)
      s->set_whitebal(s, 1);       // 0 = disable, 1 = enable
      s->set_awb_gain(s, 1);       // 0 = disable, 1 = enable
      s->set_wb_mode(s, 0);        // 0 to 4
      s->set_exposure_ctrl(s, 1);  // 0 = disable, 1 = enable
      s->set_aec2(s, 0);           // 0 = disable, 1 = enable
      s->set_ae_level(s, 0);       // -2 to 2
      s->set_aec_value(s, 400);    // Slightly higher for better power efficiency
      s->set_gain_ctrl(s, 1);      // 0 = disable, 1 = enable
      s->set_agc_gain(s, 5);       // Moderate gain to reduce sensor power
      s->set_gainceiling(s, (gainceiling_t)2); // Limited gain ceiling for power savings
      s->set_bpc(s, 0);            // 0 = disable, 1 = enable - disabled for power
      s->set_wpc(s, 1);            // 0 = disable, 1 = enable
      s->set_raw_gma(s, 1);        // 0 = disable, 1 = enable
      s->set_lenc(s, 0);           // Disabled for power savings
      s->set_hmirror(s, 0);        // 0 = disable, 1 = enable
      s->set_vflip(s, 0);          // 0 = disable, 1 = enable
      s->set_dcw(s, 1);            // 0 = disable, 1 = enable
      s->set_colorbar(s, 0);       // 0 = disable, 1 = enable

      Serial.println("Camera configured with power-optimized settings");
      return true;
    }

    Serial.printf("Camera init failed with error 0x%x\n", err);
    // Deinitialize before next attempt
    esp_camera_deinit();

    // Adjust settings for next attempt
    if (attempt == 1) {
      Serial.println("Retry with lower XCLK (10MHz)");
      config.xclk_freq_hz = 10000000;
    } else if (attempt == 2) {
      Serial.println("Retry with smaller frame size (QQVGA)");
      config.frame_size = FRAMESIZE_QQVGA; // 160x120
      config.jpeg_quality = 25;
    }
    delay(200);
  }

  Serial.println("All camera init attempts failed");
  return false;
}

void connectToNetwork() {
  // Reduce WiFi power consumption
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false); // Disable WiFi sleep for stable connection, but use power save mode
  
  Serial.printf("Connecting to WiFi SSID: %s\n", wifi_ssid);
  // Try to connect to existing WiFi first
  WiFi.begin(wifi_ssid, wifi_password);
  Serial.print("Connecting to WiFi");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 15) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi connected!");
    Serial.print("Camera IP address: ");
    Serial.println(WiFi.localIP());
    // Set WiFi power for stability on router WiFi
    WiFi.setTxPower(WIFI_POWER_19_5dBm); // Higher for reliability
    Serial.print("WiFi power set for reliability. RSSI: ");
    Serial.println(WiFi.RSSI());

    // mDNS advertise
    if (MDNS.begin("agrobot-cam")) {
      MDNS.addService("http", "tcp", 81);
      Serial.println("mDNS started: http://agrobot-cam.local:81/");
    } else {
      Serial.println("mDNS start failed");
    }
  } else {
    Serial.println();
    Serial.println("WiFi connection failed. Connecting to AgroBot hotspot...");
    
    // Use static IP so main controller can find the camera reliably
    WiFi.config(IPAddress(192,168,4,2), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
    Serial.printf("Joining AP SSID: %s\n", ap_ssid);
    WiFi.begin(ap_ssid, ap_password);
    attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println();
      Serial.println("Connected to AgroBot hotspot!");
      Serial.print("Camera IP address: ");
      Serial.println(WiFi.localIP());
      // Set lower WiFi transmission power to save energy
      WiFi.setTxPower(WIFI_POWER_11dBm);
      Serial.print("WiFi power reduced for energy efficiency. RSSI: ");
      Serial.println(WiFi.RSSI());

      // mDNS advertise on AP as well (optional)
      if (MDNS.begin("agrobot-cam")) {
        MDNS.addService("http", "tcp", 81);
        Serial.println("mDNS started: http://agrobot-cam.local:81/");
      } else {
        Serial.println("mDNS start failed");
      }
    } else {
      Serial.println();
      Serial.println("All connection attempts failed! Starting camera hotspot...");
      WiFi.mode(WIFI_AP);
      bool apStarted = WiFi.softAP(softap_ssid, softap_password);
      if (apStarted) {
        Serial.print("Camera AP SSID: ");
        Serial.println(softap_ssid);
        Serial.print("Camera AP IP address: ");
        Serial.println(WiFi.softAPIP());
      } else {
        Serial.println("Failed to start camera hotspot!");
      }
    }
  }
}

void setupServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/stream", HTTP_GET, handleStream);
  server.on("/capture", HTTP_GET, handleCapture);
  server.on("/status", HTTP_GET, handleStatus);
  
  // Camera control endpoints
  server.on("/control", HTTP_GET, handleCameraControl);
  server.on("/flash", HTTP_GET, handleFlash);

  // Diagnostics endpoint
  server.on("/diag", HTTP_GET, [](){
    String d = "{";
    d += "\"mode\":\"" + String((WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) ? "AP" : "STA") + "\",";
    d += "\"local_ip\":\"" + WiFi.localIP().toString() + "\",";
    d += "\"softap_ip\":\"" + WiFi.softAPIP().toString() + "\",";
    d += "\"rssi\":" + String(WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0) + ",";
    d += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
    d += "\"psram\":" + String(psramFound() ? 1 : 0) + ",";
    d += "\"camera\":\"" + String(camera_initialized ? "ok" : "failed") + "\"";
    d += "}";
    server.send(200, "application/json", d);
  });

  // Log 404s to Serial
  server.onNotFound([](){
    Serial.print("[HTTP 404] Path: ");
    Serial.println(server.uri());
    server.send(404, "text/plain", "Not found");
  });
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>AgroBot Camera - Power Optimized</title>
    <style>
        body { font-family: Arial, sans-serif; text-align: center; margin: 20px; background-color: #f0f8ff; }
        .header { background-color: #28a745; color: white; padding: 10px; border-radius: 5px; margin-bottom: 20px; }
        img { max-width: 100%; height: auto; border: 2px solid #333; }
        .controls { margin: 20px 0; }
        button { margin: 5px; padding: 10px 15px; font-size: 16px; }
        .status-info { background-color: #e8f5e8; padding: 15px; border-radius: 5px; margin: 10px 0; }
        .power-info { background-color: #fff3cd; padding: 10px; border-radius: 5px; margin: 10px 0; font-size: 14px; }
    </style>
</head>
<body>
    <div class="header">
        <h1>🤖 AgroBot Camera Feed</h1>
        <p>⚡ Power Management Active</p>
    </div>
    
    <div>
        <img id="stream" src="/stream" alt="Camera Stream">
    </div>
    
    <div class="controls">
        <button onclick="location.reload()">🔄 Refresh</button>
        <button onclick="capturePhoto()">📸 Capture</button>
        <button onclick="toggleFlash()">💡 Flash</button>
    </div>
    
    <div class="status-info">
        <p><strong>Status:</strong> <span id="status">Connected</span></p>
        <p><strong>Stream URL:</strong> <code>/stream</code></p>
        <p><strong>Camera IP:</strong> <code id="ip"></code></p>
        <p><strong>CPU Frequency:</strong> <span id="cpu_freq"></span> MHz</p>
        <p><strong>WiFi Power:</strong> <span id="wifi_power"></span></p>
        <p><strong>Free Memory:</strong> <span id="free_heap"></span> bytes</p>
    </div>
    
    <div class="power-info">
        <h3>🔋 Power Optimization Features</h3>
        <ul style="text-align: left; display: inline-block;">
            <li>Brownout protection enabled</li>
            <li>CPU frequency reduced to 160MHz</li>
            <li>Camera resolution optimized (CIF: 400×296)</li>
            <li>Frame rate limited to 10 FPS</li>
            <li>WiFi transmission power reduced</li>
            <li>LED power management active</li>
        </ul>
    </div>
    
    <script>
        // Update system information
        function updateStatus() {
            fetch('/status')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('ip').textContent = data.ip;
                    document.getElementById('status').textContent = data.camera_status;
                    document.getElementById('cpu_freq').textContent = data.cpu_freq;
                    document.getElementById('wifi_power').textContent = data.wifi_power;
                    document.getElementById('free_heap').textContent = data.free_heap;
                })
                .catch(error => console.log('Status update failed:', error));
        }
        
        // Initial status update
        updateStatus();
        
        // Update status every 10 seconds
        setInterval(updateStatus, 10000);
        
        function capturePhoto() {
            window.open('/capture', '_blank');
        }
        
        function toggleFlash() {
            fetch('/flash')
                .then(response => response.text())
                .then(data => alert(data));
        }
        
        // Auto refresh stream if it fails
        document.getElementById('stream').onerror = function() {
            setTimeout(() => {
                this.src = '/stream?' + new Date().getTime();
            }, 5000);
        };
    </script>
</body>
</html>
)rawliteral";
  
  server.send(200, "text/html", html);
}

void handleStream() {
  if (!camera_initialized) {
    server.send(503, "text/plain", "Camera not initialized");
    return;
  }
  
  WiFiClient client = server.client();
  Serial.print("[STREAM] Client connected: ");
  Serial.print(client.remoteIP());
  Serial.print(":");
  Serial.println(client.remotePort());
  
  // Send HTTP headers for MJPEG stream
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
  client.println("Access-Control-Allow-Origin: *");
  client.println("Cache-Control: no-cache");
  client.println("Connection: keep-alive");
  client.println();
  
  unsigned long frame_count = 0;
  unsigned long last_frame_time = 0;
  
  while (client.connected()) {
    // Power-saving: reduce frame rate for lower power consumption
    unsigned long current_time = millis();
    if (current_time - last_frame_time < 100) { // Max 10 FPS instead of ~33 FPS
      delay(10);
      continue;
    }
    last_frame_time = current_time;
    
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      break;
    }
    
    // Send frame boundary
    client.println("--frame");
    client.println("Content-Type: image/jpeg");
    client.println("Content-Length: " + String(fb->len));
    client.println();
    
    // Send image data
    client.write(fb->buf, fb->len);
    client.println();
    
    esp_camera_fb_return(fb);
    
    if (!client.connected()) {
      break;
    }
    
    frame_count++;
    // Log frame count every 25 frames for debugging
    if (frame_count % 25 == 0) {
      Serial.println("[STREAM] Frames: " + String(frame_count) + ", Free heap: " + String(ESP.getFreeHeap()));
    }
    
    delay(10); // Small delay to prevent overwhelming the system
  }
  
  client.stop();
  Serial.println("Stream ended after " + String(frame_count) + " frames");
}

void handleCapture() {
  if (!camera_initialized) {
    server.send(503, "text/plain", "Camera not initialized");
    return;
  }
  
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    server.send(500, "text/plain", "Camera capture failed");
    return;
  }
  
  server.sendHeader("Content-Disposition", "attachment; filename=agrobot_capture.jpg");
  server.send_P(200, "image/jpeg", (const char*)fb->buf, fb->len);
  
  esp_camera_fb_return(fb);
}

void handleStatus() {
  IPAddress camIp = (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) ? WiFi.softAPIP() : WiFi.localIP();
  String status = "{";
  status += "\"camera_status\":\"" + String(camera_initialized ? "Connected" : "Failed") + "\",";
  status += "\"ip\":\"" + camIp.toString() + "\",";
  status += "\"wifi_connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
  status += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
  status += "\"cpu_freq\":" + String(getCpuFrequencyMhz()) + ",";
  status += "\"wifi_power\":\"" + String(WiFi.getTxPower()) + "dBm\",";
  status += "\"power_mode\":\"optimized\",";
  status += "\"uptime\":" + String(millis());
  status += "}";
  
  server.send(200, "application/json", status);
}

void handleCameraControl() {
  String response = "Camera controls:\n";
  response += "Brightness: " + String(esp_camera_sensor_get()->status.brightness) + "\n";
  response += "Contrast: " + String(esp_camera_sensor_get()->status.contrast) + "\n";
  response += "Saturation: " + String(esp_camera_sensor_get()->status.saturation) + "\n";
  
  server.send(200, "text/plain", response);
}

void handleFlash() {
  static bool flash_on = false;
  flash_on = !flash_on;
  digitalWrite(FLASH_LED_PIN, flash_on ? HIGH : LOW);
  server.send(200, "text/plain", flash_on ? "Flash ON" : "Flash OFF");
} 