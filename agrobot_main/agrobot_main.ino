/*
 * AgroBot - Advanced Agricultural Robot
 * Main Controller (ESP32)
 * Phase 1: Basic Movement Control + Web Server
 * 
 * Features:
 * - WiFi/Hotspot mode switching
 * - Web server for robot control
 * - Motor control (Forward, Backward, Left, Right)
 * - Integration with ESP32-CAM
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>

// Motor Driver Pins (L298N)
#define MOTOR_A_PIN1 26
#define MOTOR_A_PIN2 27
#define MOTOR_A_ENABLE 14
#define MOTOR_B_PIN1 32
#define MOTOR_B_PIN2 33
#define MOTOR_B_ENABLE 25

// Motor PWM (LEDC) Configuration
const int PWM_CHANNEL_A = 0;
const int PWM_CHANNEL_B = 1;
const int PWM_FREQ = 1000;      // Hz
const int PWM_RESOLUTION = 8;   // bits (0-255 duty)

// Status LED
#define STATUS_LED 2

// Network Configuration
const char* ap_ssid = "AgroBot_Hotspot";
const char* ap_password = "agrobot123";
const char* wifi_ssid = "Compiler Error";     // Replace with your WiFi SSID
const char* wifi_password = "luffy@@12345"; // Replace with your WiFi password

// ESP32-CAM Configuration
const char* cam_ip = "192.168.4.2";  // ESP32-CAM IP when in hotspot mode
String cam_stream_url = "";

WebServer server(80);
bool hotspot_mode = false;
bool motors_enabled = true;
unsigned long lastCommand = 0;
unsigned long lastCamCheck = 0;

String resolveCameraUrl() {
  if (WiFi.status() == WL_CONNECTED) {
    // Try mDNS first
    IPAddress camIpResolved = MDNS.queryHost("agrobot-cam");
    if (camIpResolved != IPAddress(0,0,0,0)) {
      String url = String("http://") + camIpResolved.toString() + ":81/stream";
      Serial.print("Resolved camera via mDNS: ");
      Serial.println(url);
      return url;
    }
    // Fallback to configured cam_ip
    String url = String("http://") + cam_ip + ":81/stream";
    Serial.print("mDNS failed, using configured cam_ip: ");
    Serial.println(url);
    return url;
  } else {
    // AP mode: camera has static IP 192.168.4.2
    return "http://192.168.4.2:81/stream";
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("AgroBot Starting...");
  
  // Initialize pins
  setupPins();
  
  // Initialize network
  setupNetwork();
  
  // Setup web server routes
  setupWebServer();
  
  // Start web server
  server.begin();
  Serial.println("HTTP server started");
  
  // Setup mDNS
  if (MDNS.begin("agrobot")) {
    Serial.println("mDNS responder started");
    Serial.println("Access via: http://agrobot.local");
  }
  
  cam_stream_url = resolveCameraUrl();
  digitalWrite(STATUS_LED, HIGH);
  Serial.println("AgroBot Ready!");
  Serial.print("Hotspot mode: ");
  Serial.println(hotspot_mode ? "true" : "false");
  Serial.print("Camera stream URL: ");
  Serial.println(cam_stream_url);
}

void loop() {
  server.handleClient();
  
  // Auto-stop motors after 500ms of no command (safety feature)
  if (millis() - lastCommand > 500) {
    stopMotors();
  }

  // Periodically check camera reachability via /status (not /stream)
  if (millis() - lastCamCheck > 5000) {
    lastCamCheck = millis();
    if (cam_stream_url.length() > 0) {
      String statusUrl = cam_stream_url;
      int idx = statusUrl.lastIndexOf("/stream");
      if (idx > 0) {
        statusUrl = statusUrl.substring(0, idx) + "/status";
      } else {
        if (!statusUrl.endsWith("/")) statusUrl += "/";
        statusUrl += "status";
      }
      HTTPClient http;
      http.setConnectTimeout(2500);
      http.begin(statusUrl);
      int code = http.GET();
      Serial.print("[CAM CHECK] GET ");
      Serial.print(statusUrl);
      Serial.print(" => ");
      Serial.println(code);
      http.end();
      if (code <= 0) {
        Serial.println("Re-resolving camera URL...");
        cam_stream_url = resolveCameraUrl();
      }
    }
  }
  
  delay(10);
}

void setupPins() {
  // Motor pins
  pinMode(MOTOR_A_PIN1, OUTPUT);
  pinMode(MOTOR_A_PIN2, OUTPUT);
  pinMode(MOTOR_A_ENABLE, OUTPUT);
  pinMode(MOTOR_B_PIN1, OUTPUT);
  pinMode(MOTOR_B_PIN2, OUTPUT);
  pinMode(MOTOR_B_ENABLE, OUTPUT);
  
  // Status LED
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, LOW);
  
  // Setup PWM for motor enable pins
  ledcSetup(PWM_CHANNEL_A, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(MOTOR_A_ENABLE, PWM_CHANNEL_A);
  ledcSetup(PWM_CHANNEL_B, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(MOTOR_B_ENABLE, PWM_CHANNEL_B);
  
  // Initialize motors (stopped)
  stopMotors();
}

void setupNetwork() {
  // Start AP first so ESP32-CAM has a network to join
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ap_ssid, ap_password);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  hotspot_mode = true;
 
  // Then try to connect STA to router (optional)
  WiFi.begin(wifi_ssid, wifi_password);
  Serial.print("Connecting to WiFi");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    hotspot_mode = false;
  } else {
    Serial.println();
    Serial.println("Router connection failed. Staying in AP mode.");
  }
}

void setupHotspot() {
  // Kept for compatibility; now handled in setupNetwork()
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ap_ssid, ap_password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  hotspot_mode = true;
  cam_stream_url = "http://192.168.4.2:81/stream";
}

void setupWebServer() {
  // Serve main control page
  server.on("/", handleRoot);
  
  // Motor control endpoints
  server.on("/forward", handleForward);
  server.on("/backward", handleBackward);
  server.on("/left", handleLeft);
  server.on("/right", handleRight);
  server.on("/stop", handleStop);
  
  // Status endpoints
  server.on("/status", handleStatus);
  server.on("/network-info", handleNetworkInfo);

  // Diagnostics endpoint
  server.on("/diag", HTTP_GET, [](){
    String info = "{";
    info += "\"hotspot_mode\":" + String(hotspot_mode ? "true" : "false") + ",";
    info += "\"ip\":\"" + (hotspot_mode ? WiFi.softAPIP().toString() : WiFi.localIP().toString()) + "\",";
    info += "\"cam_stream_url\":\"" + cam_stream_url + "\"";
    info += "}";
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", info);
  });
  
  // Enable/disable motors
  server.on("/toggle-motors", handleToggleMotors);
  
  // Handle CORS for all requests
  server.onNotFound(handleCORS);
}

void handleRoot() {
  String html = getMainHTML();
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/html", html);
}

void handleForward() {
  if (motors_enabled) {
    moveForward();
    lastCommand = millis();
    Serial.println("Moving Forward");
  }
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "Moving Forward");
}

void handleBackward() {
  if (motors_enabled) {
    moveBackward();
    lastCommand = millis();
    Serial.println("Moving Backward");
  }
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "Moving Backward");
}

void handleLeft() {
  if (motors_enabled) {
    turnLeft();
    lastCommand = millis();
    Serial.println("Turning Left");
  }
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "Turning Left");
}

void handleRight() {
  if (motors_enabled) {
    turnRight();
    lastCommand = millis();
    Serial.println("Turning Right");
  }
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "Turning Right");
}

void handleStop() {
  stopMotors();
  Serial.println("Motors Stopped");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "Motors Stopped");
}

void handleStatus() {
  String response = "{";
  response += "\"wifi_connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
  response += "\"hotspot_mode\":" + String(hotspot_mode ? "true" : "false") + ",";
  response += "\"motors_enabled\":" + String(motors_enabled ? "true" : "false") + ",";
  response += "\"autonomous_mode\":" + String(autonomous_mode ? "true" : "false") + ",";
  response += "\"pump_active\":" + String(pump_active ? "true" : "false") + ",";
  response += "\"ip_address\":\"" + (hotspot_mode ? WiFi.softAPIP().toString() : WiFi.localIP().toString()) + "\",";
  response += "\"cam_stream_url\":\"" + cam_stream_url + "\",";
  response += "\"uptime\":" + String(millis()) + ",";
  response += "\"free_heap\":" + String(ESP.getFreeHeap());
  response += "}";
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", response);
}

void handleNetworkInfo() {
  String info = "{";
  info += "\"mode\":\"" + String(hotspot_mode ? "Hotspot" : "WiFi") + "\",";
  info += "\"ip\":\"" + (hotspot_mode ? WiFi.softAPIP().toString() : WiFi.localIP().toString()) + "\",";
  info += "\"connected_clients\":" + String(WiFi.softAPgetStationNum());
  info += "}";
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", info);
}

void handleToggleMotors() {
  motors_enabled = !motors_enabled;
  if (!motors_enabled) {
    stopMotors();
  }
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", motors_enabled ? "Motors Enabled" : "Motors Disabled");
}

void handleCORS() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.send(204);
}

// Motor Control Functions
void moveForward() {
  // Left motor forward
  digitalWrite(MOTOR_A_PIN1, HIGH);
  digitalWrite(MOTOR_A_PIN2, LOW);
  ledcWrite(PWM_CHANNEL_A, 200); // Speed control (0-255)
  
  // Right motor forward
  digitalWrite(MOTOR_B_PIN1, HIGH);
  digitalWrite(MOTOR_B_PIN2, LOW);
  ledcWrite(PWM_CHANNEL_B, 200);
}

void moveBackward() {
  // Left motor backward
  digitalWrite(MOTOR_A_PIN1, LOW);
  digitalWrite(MOTOR_A_PIN2, HIGH);
  ledcWrite(PWM_CHANNEL_A, 200);
  
  // Right motor backward
  digitalWrite(MOTOR_B_PIN1, LOW);
  digitalWrite(MOTOR_B_PIN2, HIGH);
  ledcWrite(PWM_CHANNEL_B, 200);
}

void turnLeft() {
  // Left motor backward, Right motor forward
  digitalWrite(MOTOR_A_PIN1, LOW);
  digitalWrite(MOTOR_A_PIN2, HIGH);
  ledcWrite(PWM_CHANNEL_A, 150);
  
  digitalWrite(MOTOR_B_PIN1, HIGH);
  digitalWrite(MOTOR_B_PIN2, LOW);
  ledcWrite(PWM_CHANNEL_B, 150);
}

void turnRight() {
  // Left motor forward, Right motor backward
  digitalWrite(MOTOR_A_PIN1, HIGH);
  digitalWrite(MOTOR_A_PIN2, LOW);
  ledcWrite(PWM_CHANNEL_A, 150);
  
  digitalWrite(MOTOR_B_PIN1, LOW);
  digitalWrite(MOTOR_B_PIN2, HIGH);
  ledcWrite(PWM_CHANNEL_B, 150);
}

void stopMotors() {
  digitalWrite(MOTOR_A_PIN1, LOW);
  digitalWrite(MOTOR_A_PIN2, LOW);
  ledcWrite(PWM_CHANNEL_A, 0);
  
  digitalWrite(MOTOR_B_PIN1, LOW);
  digitalWrite(MOTOR_B_PIN2, LOW);
  ledcWrite(PWM_CHANNEL_B, 0);
}

String getMainHTML() {
  return R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>AgroBot Controller</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: 'Arial', sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            color: white;
        }
        
        .container {
            max-width: 1200px;
            margin: 0 auto;
            padding: 20px;
        }
        
        .header {
            text-align: center;
            margin-bottom: 30px;
        }
        
        .header h1 {
            font-size: 2.5rem;
            margin-bottom: 10px;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
        }
        
        .status-bar {
            background: rgba(255, 255, 255, 0.1);
            backdrop-filter: blur(10px);
            padding: 15px;
            border-radius: 15px;
            margin-bottom: 20px;
            display: flex;
            justify-content: space-between;
            align-items: center;
            flex-wrap: wrap;
        }
        
        .main-content {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 20px;
        }
        
        .video-section {
            background: rgba(0, 0, 0, 0.3);
            padding: 20px;
            border-radius: 15px;
            backdrop-filter: blur(10px);
        }
        
        .video-container {
            position: relative;
            width: 100%;
            height: 300px;
            background: #000;
            border-radius: 10px;
            overflow: hidden;
        }
        
        #videoStream {
            width: 100%;
            height: 100%;
            object-fit: cover;
        }
        
        .control-section {
            background: rgba(255, 255, 255, 0.1);
            padding: 20px;
            border-radius: 15px;
            backdrop-filter: blur(10px);
        }
        
        .control-grid {
            display: grid;
            grid-template-columns: 1fr 1fr 1fr;
            grid-template-rows: 1fr 1fr 1fr;
            gap: 15px;
            height: 300px;
        }
        
        .control-btn {
            background: linear-gradient(145deg, #667eea, #764ba2);
            border: none;
            border-radius: 15px;
            color: white;
            font-size: 1.2rem;
            font-weight: bold;
            cursor: pointer;
            transition: all 0.3s ease;
            box-shadow: 0 4px 15px rgba(0, 0, 0, 0.2);
            display: flex;
            align-items: center;
            justify-content: center;
        }
        
        .control-btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 6px 20px rgba(0, 0, 0, 0.3);
        }
        
        .control-btn:active {
            transform: translateY(0);
            box-shadow: 0 2px 10px rgba(0, 0, 0, 0.2);
        }
        
        .forward { grid-column: 2; grid-row: 1; }
        .left { grid-column: 1; grid-row: 2; }
        .stop { grid-column: 2; grid-row: 2; background: linear-gradient(145deg, #ff6b6b, #ee5a24); }
        .right { grid-column: 3; grid-row: 2; }
        .backward { grid-column: 2; grid-row: 3; }
        
        .info-panel {
            margin-top: 20px;
            background: rgba(0, 0, 0, 0.2);
            padding: 15px;
            border-radius: 10px;
        }
        
        .toggle-motors {
            background: linear-gradient(145deg, #2ed573, #1e90ff);
            border: none;
            padding: 10px 20px;
            border-radius: 25px;
            color: white;
            font-weight: bold;
            cursor: pointer;
            transition: all 0.3s ease;
        }
        
        @media (max-width: 768px) {
            .main-content {
                grid-template-columns: 1fr;
            }
            
            .status-bar {
                flex-direction: column;
                gap: 10px;
            }
            
            .header h1 {
                font-size: 2rem;
            }
        }
        
        .loading {
            display: inline-block;
            width: 20px;
            height: 20px;
            border: 3px solid #f3f3f3;
            border-top: 3px solid #667eea;
            border-radius: 50%;
            animation: spin 1s linear infinite;
        }
        
        @keyframes spin {
            0% { transform: rotate(0deg); }
            100% { transform: rotate(360deg); }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🤖 AgroBot Controller</h1>
            <p>Advanced Agricultural Robot Control System</p>
        </div>
        
        <div class="status-bar">
            <div>
                <span id="connectionStatus">Connecting...</span>
            </div>
            <div>
                <button class="toggle-motors" onclick="toggleMotors()">Toggle Motors</button>
            </div>
            <div>
                <span id="networkInfo">Loading network info...</span>
            </div>
        </div>
        
        <div class="main-content">
            <div class="video-section">
                <h3>📹 Live Camera Feed</h3>
                <div class="video-container">
                    <img id="videoStream" src="" alt="Camera feed will appear here when ESP32-CAM is connected">
                </div>
                <div class="info-panel">
                    <p><strong>Camera Status:</strong> <span id="cameraStatus">Checking...</span></p>
                    <p><strong>Stream URL:</strong> <span id="streamUrl">Loading...</span></p>
                </div>
            </div>
            
            <div class="control-section">
                <h3>🎮 Movement Controls</h3>
                <div class="control-grid">
                    <button class="control-btn forward" onmousedown="startMoving('forward')" onmouseup="stopMoving()" ontouchstart="startMoving('forward')" ontouchend="stopMoving()">
                        ⬆️ FORWARD
                    </button>
                    <button class="control-btn left" onmousedown="startMoving('left')" onmouseup="stopMoving()" ontouchstart="startMoving('left')" ontouchend="stopMoving()">
                        ⬅️ LEFT
                    </button>
                    <button class="control-btn stop" onclick="stopMoving()">
                        🛑 STOP
                    </button>
                    <button class="control-btn right" onmousedown="startMoving('right')" onmouseup="stopMoving()" ontouchstart="startMoving('right')" ontouchend="stopMoving()">
                        ➡️ RIGHT
                    </button>
                    <button class="control-btn backward" onmousedown="startMoving('backward')" onmouseup="stopMoving()" ontouchstart="startMoving('backward')" ontouchend="stopMoving()">
                        ⬇️ BACKWARD
                    </button>
                </div>
                
                <div class="info-panel">
                    <p><strong>Motors:</strong> <span id="motorStatus">Enabled</span></p>
                    <p><strong>Last Command:</strong> <span id="lastCommand">None</span></p>
                </div>
            </div>
        </div>
    </div>
    
    <script>
        let movingInterval = null;
        let currentDirection = null;
        
        // Initialize on page load
        document.addEventListener('DOMContentLoaded', function() {
            updateStatus();
            initializeCamera();
            setInterval(updateStatus, 5000); // Update status every 5 seconds
        });
        
        async function updateStatus() {
            try {
                const response = await fetch('/status');
                const status = await response.json();
                
                document.getElementById('connectionStatus').innerHTML = 
                    status.wifi_connected ? '🟢 WiFi Connected' : '🟡 Hotspot Mode';
                
                document.getElementById('motorStatus').innerHTML = 
                    status.motors_enabled ? '🟢 Enabled' : '🔴 Disabled';
                    
                document.getElementById('streamUrl').textContent = status.cam_stream_url;
                
                // Try to load camera stream
                if (status.cam_stream_url) {
                    document.getElementById('videoStream').src = status.cam_stream_url;
                }
                
            } catch (error) {
                document.getElementById('connectionStatus').innerHTML = '🔴 Connection Error';
                console.error('Status update failed:', error);
            }
            
            try {
                const netResponse = await fetch('/network-info');
                const netInfo = await netResponse.json();
                document.getElementById('networkInfo').innerHTML = 
                    `${netInfo.mode}: ${netInfo.ip}`;
            } catch (error) {
                console.error('Network info update failed:', error);
            }
        }
        
        async function initializeCamera() {
            const video = document.getElementById('videoStream');
            const status = document.getElementById('cameraStatus');
            
            video.onload = function() {
                status.textContent = '🟢 Connected';
            };
            
            video.onerror = function() {
                status.textContent = '🔴 Not Connected';
                video.alt = 'Camera not available. Please ensure ESP32-CAM is connected and running.';
            };
        }
        
        function startMoving(direction) {
            if (currentDirection === direction) return;
            
            currentDirection = direction;
            document.getElementById('lastCommand').textContent = direction.toUpperCase();
            
            // Send initial command
            sendCommand(direction);
            
            // Send continuous commands while button is held
            movingInterval = setInterval(() => {
                sendCommand(direction);
            }, 100);
        }
        
        function stopMoving() {
            currentDirection = null;
            if (movingInterval) {
                clearInterval(movingInterval);
                movingInterval = null;
            }
            sendCommand('stop');
            document.getElementById('lastCommand').textContent = 'STOP';
        }
        
        async function sendCommand(command) {
            try {
                const response = await fetch(`/${command}`, {
                    method: 'GET'
                });
                
                if (!response.ok) {
                    throw new Error(`Command failed: ${response.status}`);
                }
                
            } catch (error) {
                console.error('Command failed:', error);
                document.getElementById('connectionStatus').innerHTML = '🔴 Command Failed';
            }
        }
        
        async function toggleMotors() {
            try {
                const response = await fetch('/toggle-motors');
                const result = await response.text();
                
                // Update status immediately
                setTimeout(updateStatus, 100);
                
            } catch (error) {
                console.error('Toggle motors failed:', error);
            }
        }
        
        // Prevent context menu on long press (mobile)
        document.addEventListener('contextmenu', function(e) {
            e.preventDefault();
        });
        
        // Prevent selection
        document.addEventListener('selectstart', function(e) {
            e.preventDefault();
        });
    </script>
</body>
</html>
)rawliteral";
} 

void handleSettings() {
  String response = "{";
  response += "\"obstacle_threshold\":" + String(obstacle_distance_threshold) + ",";
  response += "\"soil_moisture_threshold\":" + String(soil_moisture_threshold) + ",";
  response += "\"water_level_threshold\":" + String(water_level_threshold);
  response += "}";
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", response);
}