/*
 * AgroBot - Advanced Agricultural Robot
 * Phase 2: Complete Sensor Integration
 * 
 * Features:
 * - All Phase 1 features (movement, video, web control)
 * - DHT11 temperature/humidity monitoring
 * - Soil moisture detection
 * - Ultrasonic obstacle avoidance
 * - Automated water pump control
 * - Water level monitoring
 * - Servo-controlled camera positioning
 * - Data logging and analytics
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <DHT.h>
#include <ESP32Servo.h>
#include <HTTPClient.h>

// Motor Driver Pins (L298N)
#define MOTOR_A_PIN1 26
#define MOTOR_A_PIN2 27
#define MOTOR_A_ENABLE 14
#define MOTOR_B_PIN1 32
#define MOTOR_B_PIN2 33
#define MOTOR_B_ENABLE 25

// Sensor Pins
#define DHT_PIN 4
#define DHT_TYPE DHT11
#define ULTRASONIC_TRIG 18
#define ULTRASONIC_ECHO 19
#define WATER_LEVEL_PIN 34

// Control Pins
#define PUMP_RELAY_PIN 22
#define SERVO_PIN 23
#define STATUS_LED 2

// Network Configuration
const char* ap_ssid = "AgroBot_Hotspot";
const char* ap_password = "agrobot123";
const char* wifi_ssid = "Compiler Error";     // Replace with your WiFi SSID
const char* wifi_password = "luffy@@12345"; // Replace with your WiFi password

// ESP32-CAM Configuration
const char* cam_ip = "192.168.4.2";
String cam_stream_url = "";

// LEDC PWM for motors
const int PWM_CHANNEL_A = 0;
const int PWM_CHANNEL_B = 1;
const int PWM_FREQ = 1000;
const int PWM_RESOLUTION = 8;

unsigned long lastCamCheck = 0;

// Objects
WebServer server(80);
DHT dht(DHT_PIN, DHT_TYPE);
Servo cameraServo;

// System State
bool hotspot_mode = false;
bool motors_enabled = true;
bool autonomous_mode = false;
bool pump_active = false;
int servo_position = 90; // Center position

// Sensor Data Structure
struct SensorData {
  float temperature;
  float humidity;
  int soil_moisture;
  float distance;
  int water_level;
  unsigned long timestamp;
};

SensorData currentData;

// Autonomous Settings
int obstacle_distance_threshold = 20; // cm
int soil_moisture_threshold = 30; // %
int water_level_threshold = 20; // %

// Timing
unsigned long lastSensorRead = 0;
unsigned long lastCommand = 0;
unsigned long pumpStartTime = 0;
const unsigned long sensorInterval = 2000; // 2 seconds
const unsigned long maxPumpTime = 10000; // 10 seconds max pump time

// Pins
#define SOIL_MOISTURE_PIN 35   // analog-only input pin
#define SOIL_POWER_PIN    13   // any free GPIO to power the sensor

// Calibration (measure these on your hardware)
const int SOIL_RAW_DRY   = 3500;  // reading in air (dry)
const int SOIL_RAW_WET   = 1500;  // reading fully dipped in water (wet)

void setup() {
  Serial.begin(115200);
  Serial.println("AgroBot Starting...");
  
  // Initialize pins and components
  setupPins();
  initializeSensors();
  
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
  }
  
  digitalWrite(STATUS_LED, HIGH);
  Serial.println("AgroBot Ready!");
}

String resolveCameraUrl() {
  if (WiFi.status() == WL_CONNECTED) {
    IPAddress camIpResolved = MDNS.queryHost("agrobot-cam");
    if (camIpResolved != IPAddress(0,0,0,0)) {
      String url = String("http://") + camIpResolved.toString() + ":81/stream";
      Serial.print("Resolved camera via mDNS: ");
      Serial.println(url);
      return url;
    }
    String url = String("http://") + cam_ip + ":81/stream";
    Serial.print("mDNS failed, using configured cam_ip: ");
    Serial.println(url);
    return url;
  } else {
    return "http://192.168.4.2:81/stream";
  }
}

void loop() {
  server.handleClient();
  
  // Read sensors periodically
  if (millis() - lastSensorRead > sensorInterval) {
    readAllSensors();
    lastSensorRead = millis();
  }
  
  // Autonomous mode operations
  if (autonomous_mode) {
    performAutonomousOperations();
  }
  
  // Safety: Auto-stop motors after inactivity
  if (millis() - lastCommand > 500 && !autonomous_mode) {
    stopMotors();
  }
  
  // Periodic camera health check via /status
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
  
  // Pump safety: Auto-stop after max time
  if (pump_active && (millis() - pumpStartTime > maxPumpTime)) {
    stopPump();
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

  // Setup PWM for motor enable pins
  ledcSetup(PWM_CHANNEL_A, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(MOTOR_A_ENABLE, PWM_CHANNEL_A);
  ledcSetup(PWM_CHANNEL_B, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(MOTOR_B_ENABLE, PWM_CHANNEL_B);
  
  // Control pins
  pinMode(PUMP_RELAY_PIN, OUTPUT);
  pinMode(STATUS_LED, OUTPUT);
  pinMode(ULTRASONIC_TRIG, OUTPUT);
  pinMode(ULTRASONIC_ECHO, INPUT);
  
  // Initialize outputs
  digitalWrite(PUMP_RELAY_PIN, LOW);
  digitalWrite(STATUS_LED, LOW);
  stopMotors();

  // Power pin for soil moisture sensor
  pinMode(SOIL_POWER_PIN, OUTPUT);
  digitalWrite(SOIL_POWER_PIN, LOW); // keep off when idle
  analogSetPinAttenuation(SOIL_MOISTURE_PIN, ADC_11db); // up to ~3.6V range
}

void initializeSensors() {
  // Initialize DHT sensor
  dht.begin();
  
  // Initialize servo
  cameraServo.attach(SERVO_PIN);
  cameraServo.write(servo_position);
  
  // Wait for sensors to stabilize
  delay(2000);
  
  // Initial sensor reading
  readAllSensors();
  
  Serial.println("Sensors initialized successfully");
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
    cam_stream_url = resolveCameraUrl();
  } else {
    Serial.println();
    Serial.println("Router connection failed. Staying in AP mode.");
    cam_stream_url = "http://192.168.4.2:81/stream";
  }
}

void setupHotspot() {
  WiFi.softAP(ap_ssid, ap_password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  hotspot_mode = true;
  cam_stream_url = "http://192.168.4.2:81/stream";
}

void setupWebServer() {
  // Main page
  server.on("/", handleRoot);
  
  // Movement controls
  server.on("/forward", handleForward);
  server.on("/backward", handleBackward);
  server.on("/left", handleLeft);
  server.on("/right", handleRight);
  server.on("/stop", handleStop);
  
  // System controls
  server.on("/toggle-motors", handleToggleMotors);
  server.on("/toggle-autonomous", handleToggleAutonomous);
  
  // Pump controls
  server.on("/pump-on", handlePumpOn);
  server.on("/pump-off", handlePumpOff);
  
  // Camera controls
  server.on("/servo-left", handleServoLeft);
  server.on("/servo-right", handleServoRight);
  server.on("/servo-center", handleServoCenter);
  
  // Data endpoints
  server.on("/status", handleStatus);
  server.on("/sensor-data", handleSensorData);
  server.on("/settings", handleSettings);
  
  // Settings updates
  server.on("/update-thresholds", HTTP_POST, handleUpdateThresholds);
  
  server.onNotFound(handleCORS);
}

void readAllSensors() {
  // Read DHT11
  currentData.temperature = dht.readTemperature();
  currentData.humidity = dht.readHumidity();
  
  // Read soil moisture (convert to percentage)
  int soilRaw = readSoilRaw();
  currentData.soil_moisture = readSoilMoisturePercent();
  
  // Read ultrasonic distance
  currentData.distance = readUltrasonicDistance();
  
  // Read water level (convert to percentage)
  int waterRaw = analogRead(WATER_LEVEL_PIN);
  currentData.water_level = map(waterRaw, 0, 4095, 0, 100);
  
  currentData.timestamp = millis();
  
  // Debug output
  Serial.printf("Temp: %.1f°C, Humidity: %.1f%%, Soil: %d%%, Distance: %.1fcm, Water: %d%%\n",
    currentData.temperature, currentData.humidity, currentData.soil_moisture, 
    currentData.distance, currentData.water_level);
}

float readUltrasonicDistance() {
  digitalWrite(ULTRASONIC_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRASONIC_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRASONIC_TRIG, LOW);
  
  long duration = pulseIn(ULTRASONIC_ECHO, HIGH, 30000); // 30ms timeout
  if (duration == 0) return 999; // No echo received
  
  return duration * 0.034 / 2; // Convert to cm
}

int readSoilRaw() {
  digitalWrite(SOIL_POWER_PIN, HIGH);
  delay(100); // settle
  const int samples = 10;
  long sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(SOIL_MOISTURE_PIN);
    delay(5);
  }
  digitalWrite(SOIL_POWER_PIN, LOW);
  return (int)(sum / samples);
}

int readSoilMoisturePercent() {
  int raw = readSoilRaw();
  // Many resistive sensors read LOWER when wetter; adjust if reversed
  int pct = map(raw, SOIL_RAW_DRY, SOIL_RAW_WET, 0, 100);
  pct = constrain(pct, 0, 100);
  return pct;
}

void performAutonomousOperations() {
  // Obstacle avoidance
  if (currentData.distance < obstacle_distance_threshold && currentData.distance > 0) {
    // Stop and turn to avoid obstacle
    stopMotors();
    delay(500);
    turnRight();
    delay(1000);
    stopMotors();
    Serial.println("Obstacle detected - avoiding");
    return;
  }
  
  // Automatic watering based on soil moisture
  if (currentData.soil_moisture < soil_moisture_threshold && 
      currentData.water_level > water_level_threshold && 
      !pump_active) {
    startPump();
    Serial.println("Auto-watering: Soil moisture low");
  }
  
  // Stop pump if soil is adequately moist or water level is low
  if (pump_active && 
      (currentData.soil_moisture > soil_moisture_threshold + 10 || 
       currentData.water_level < water_level_threshold)) {
    stopPump();
    Serial.println("Auto-watering: Stopping pump");
  }
}

// Motor Control Functions
void moveForward() {
  if (!checkObstacle()) {
    digitalWrite(MOTOR_A_PIN1, HIGH);
    digitalWrite(MOTOR_A_PIN2, LOW);
    ledcWrite(PWM_CHANNEL_A, 200);
    
    digitalWrite(MOTOR_B_PIN1, HIGH);
    digitalWrite(MOTOR_B_PIN2, LOW);
    ledcWrite(PWM_CHANNEL_B, 200);
    
    lastCommand = millis();
  }
}

void moveBackward() {
  digitalWrite(MOTOR_A_PIN1, LOW);
  digitalWrite(MOTOR_A_PIN2, HIGH);
  ledcWrite(PWM_CHANNEL_A, 200);
  
  digitalWrite(MOTOR_B_PIN1, LOW);
  digitalWrite(MOTOR_B_PIN2, HIGH);
  ledcWrite(PWM_CHANNEL_B, 200);
  
  lastCommand = millis();
}

void turnLeft() {
  digitalWrite(MOTOR_A_PIN1, LOW);
  digitalWrite(MOTOR_A_PIN2, HIGH);
  ledcWrite(PWM_CHANNEL_A, 150);
  
  digitalWrite(MOTOR_B_PIN1, HIGH);
  digitalWrite(MOTOR_B_PIN2, LOW);
  ledcWrite(PWM_CHANNEL_B, 150);
  
  lastCommand = millis();
}

void turnRight() {
  digitalWrite(MOTOR_A_PIN1, HIGH);
  digitalWrite(MOTOR_A_PIN2, LOW);
  ledcWrite(PWM_CHANNEL_A, 150);
  
  digitalWrite(MOTOR_B_PIN1, LOW);
  digitalWrite(MOTOR_B_PIN2, HIGH);
  ledcWrite(PWM_CHANNEL_B, 150);
  
  lastCommand = millis();
}

void stopMotors() {
  digitalWrite(MOTOR_A_PIN1, LOW);
  digitalWrite(MOTOR_A_PIN2, LOW);
  ledcWrite(PWM_CHANNEL_A, 0);
  
  digitalWrite(MOTOR_B_PIN1, LOW);
  digitalWrite(MOTOR_B_PIN2, LOW);
  ledcWrite(PWM_CHANNEL_B, 0);
}

bool checkObstacle() {
  if (currentData.distance < obstacle_distance_threshold && currentData.distance > 0) {
    Serial.println("Obstacle detected - movement blocked");
    return true;
  }
  return false;
}

// Pump Control Functions
void startPump() {
  digitalWrite(PUMP_RELAY_PIN, HIGH);
  pump_active = true;
  pumpStartTime = millis();
  Serial.println("Water pump started");
}

void stopPump() {
  digitalWrite(PUMP_RELAY_PIN, LOW);
  pump_active = false;
  Serial.println("Water pump stopped");
}

// Servo Control Functions
void moveServo(int position) {
  position = constrain(position, 0, 180);
  servo_position = position;
  cameraServo.write(position);
  delay(500); // Allow time for movement
}

// Web Handlers
void handleRoot() {
  String html = getAdvancedHTML();
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/html", html);
}

void handleForward() {
  if (motors_enabled) {
    moveForward();
    Serial.println("Moving Forward");
  }
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "Moving Forward");
}

void handleBackward() {
  if (motors_enabled) {
    moveBackward();
    Serial.println("Moving Backward");
  }
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "Moving Backward");
}

void handleLeft() {
  if (motors_enabled) {
    turnLeft();
    Serial.println("Turning Left");
  }
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "Turning Left");
}

void handleRight() {
  if (motors_enabled) {
    turnRight();
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

void handleToggleMotors() {
  motors_enabled = !motors_enabled;
  if (!motors_enabled) {
    stopMotors();
  }
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", motors_enabled ? "Motors Enabled" : "Motors Disabled");
}

void handleToggleAutonomous() {
  autonomous_mode = !autonomous_mode;
  if (!autonomous_mode) {
    stopMotors();
    stopPump();
  }
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", autonomous_mode ? "Autonomous Mode ON" : "Autonomous Mode OFF");
}

void handlePumpOn() {
  if (currentData.water_level > water_level_threshold) {
    startPump();
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", "Pump Started");
  } else {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(400, "text/plain", "Water level too low");
  }
}

void handlePumpOff() {
  stopPump();
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "Pump Stopped");
}

void handleServoLeft() {
  moveServo(servo_position - 30);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "Camera Left");
}

void handleServoRight() {
  moveServo(servo_position + 30);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "Camera Right");
}

void handleServoCenter() {
  moveServo(90);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "Camera Center");
}

void handleSensorData() {
  String response = "{";
  response += "\"temperature\":" + String(currentData.temperature, 1) + ",";
  response += "\"humidity\":" + String(currentData.humidity, 1) + ",";
  response += "\"soil_moisture\":" + String(currentData.soil_moisture) + ",";
  response += "\"distance\":" + String(currentData.distance, 1) + ",";
  response += "\"water_level\":" + String(currentData.water_level) + ",";
  response += "\"timestamp\":" + String(currentData.timestamp) + ",";
  response += "\"pump_active\":" + String(pump_active ? "true" : "false") + ",";
  response += "\"servo_position\":" + String(servo_position);
  response += "}";
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", response);
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

void handleSettings() {
  String response = "{";
  response += "\"obstacle_threshold\":" + String(obstacle_distance_threshold) + ",";
  response += "\"soil_moisture_threshold\":" + String(soil_moisture_threshold) + ",";
  response += "\"water_level_threshold\":" + String(water_level_threshold);
  response += "}";
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", response);
}

void handleUpdateThresholds() {
  if (server.hasArg("obstacle")) {
    obstacle_distance_threshold = server.arg("obstacle").toInt();
  }
  if (server.hasArg("soil")) {
    soil_moisture_threshold = server.arg("soil").toInt();
  }
  if (server.hasArg("water")) {
    water_level_threshold = server.arg("water").toInt();
  }
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "Thresholds updated");
}

void handleCORS() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.send(204);
}

String getAdvancedHTML() {
  return R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>AgroBot Advanced Controller</title>
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
            max-width: 1400px;
            margin: 0 auto;
            padding: 20px;
        }
        
        .header {
            text-align: center;
            margin-bottom: 20px;
        }
        
        .header h1 {
            font-size: 2.2rem;
            margin-bottom: 5px;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
        }
        
        .status-bar {
            background: rgba(255, 255, 255, 0.1);
            backdrop-filter: blur(10px);
            padding: 15px;
            border-radius: 15px;
            margin-bottom: 20px;
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 15px;
            align-items: center;
        }
        
        .main-grid {
            display: grid;
            grid-template-columns: 1fr 1fr 1fr;
            gap: 20px;
            margin-bottom: 20px;
        }
        
        .panel {
            background: rgba(255, 255, 255, 0.1);
            backdrop-filter: blur(10px);
            padding: 20px;
            border-radius: 15px;
        }
        
        .video-panel {
            grid-column: span 1;
        }
        
        .control-panel {
            grid-column: span 1;
        }
        
        .sensor-panel {
            grid-column: span 1;
        }
        
        .video-container {
            position: relative;
            width: 100%;
            height: 250px;
            background: #000;
            border-radius: 10px;
            overflow: hidden;
            margin-bottom: 15px;
        }
        
        #videoStream {
            width: 100%;
            height: 100%;
            object-fit: cover;
        }
        
        .camera-controls {
            display: flex;
            justify-content: center;
            gap: 10px;
            margin-top: 10px;
        }
        
        .control-grid {
            display: grid;
            grid-template-columns: 1fr 1fr 1fr;
            grid-template-rows: 1fr 1fr 1fr;
            gap: 10px;
            height: 250px;
            margin-bottom: 15px;
        }
        
        .control-btn {
            background: linear-gradient(145deg, #667eea, #764ba2);
            border: none;
            border-radius: 12px;
            color: white;
            font-size: 1rem;
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
        
        .forward { grid-column: 2; grid-row: 1; }
        .left { grid-column: 1; grid-row: 2; }
        .stop { grid-column: 2; grid-row: 2; background: linear-gradient(145deg, #ff6b6b, #ee5a24); }
        .right { grid-column: 3; grid-row: 2; }
        .backward { grid-column: 2; grid-row: 3; }
        
        .system-controls {
            display: flex;
            flex-wrap: wrap;
            gap: 10px;
            justify-content: center;
        }
        
        .system-btn {
            background: linear-gradient(145deg, #2ed573, #1e90ff);
            border: none;
            padding: 8px 16px;
            border-radius: 20px;
            color: white;
            font-size: 0.9rem;
            font-weight: bold;
            cursor: pointer;
            transition: all 0.3s ease;
        }
        
        .sensor-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 15px;
            margin-bottom: 15px;
        }
        
        .sensor-card {
            background: rgba(0, 0, 0, 0.2);
            padding: 15px;
            border-radius: 10px;
            text-align: center;
        }
        
        .sensor-value {
            font-size: 1.5rem;
            font-weight: bold;
            margin: 5px 0;
        }
        
        .sensor-label {
            font-size: 0.9rem;
            opacity: 0.8;
        }
        
        .pump-controls {
            text-align: center;
            margin-top: 15px;
        }
        
        .pump-btn {
            background: linear-gradient(145deg, #3742fa, #2f3542);
            border: none;
            padding: 10px 20px;
            margin: 5px;
            border-radius: 25px;
            color: white;
            font-weight: bold;
            cursor: pointer;
        }
        
        .pump-active {
            background: linear-gradient(145deg, #ff6b6b, #ee5a24);
        }
        
        .settings-panel {
            grid-column: span 3;
            margin-top: 20px;
        }
        
        .threshold-controls {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
            gap: 20px;
        }
        
        .threshold-group {
            background: rgba(0, 0, 0, 0.2);
            padding: 15px;
            border-radius: 10px;
        }
        
        .threshold-input {
            width: 100%;
            padding: 8px;
            margin: 5px 0;
            border: none;
            border-radius: 5px;
            background: rgba(255, 255, 255, 0.2);
            color: white;
        }
        
        .threshold-input::placeholder {
            color: rgba(255, 255, 255, 0.7);
        }
        
        @media (max-width: 1024px) {
            .main-grid {
                grid-template-columns: 1fr;
            }
            
            .video-panel, .control-panel, .sensor-panel {
                grid-column: span 1;
            }
            
            .settings-panel {
                grid-column: span 1;
            }
        }
        
        .status-indicator {
            width: 12px;
            height: 12px;
            border-radius: 50%;
            display: inline-block;
            margin-right: 8px;
        }
        
        .status-green { background: #2ed573; }
        .status-yellow { background: #ffa502; }
        .status-red { background: #ff3742; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🤖 AgroBot Advanced Controller</h1>
            <p>Agricultural Robot System</p>
        </div>
        
        <div class="status-bar">
            <div>
                <span class="status-indicator" id="connectionIndicator"></span>
                <span id="connectionStatus">Connecting...</span>
            </div>
            <div>
                <span class="status-indicator" id="motorIndicator"></span>
                <span id="motorStatus">Motors</span>
            </div>
            <div>
                <span class="status-indicator" id="autonomousIndicator"></span>
                <span id="autonomousStatus">Manual Mode</span>
            </div>
            <div>
                <span class="status-indicator" id="pumpIndicator"></span>
                <span id="pumpStatus">Pump Off</span>
            </div>
        </div>
        
        <div class="main-grid">
            <div class="panel video-panel">
                <h3>📹 Live Camera Feed</h3>
                <div class="video-container">
                    <img id="videoStream" src="" alt="Camera feed loading...">
                </div>
                <div class="camera-controls">
                    <button class="system-btn" onclick="moveCamera('left')">⬅️ Left</button>
                    <button class="system-btn" onclick="moveCamera('center')">🎯 Center</button>
                    <button class="system-btn" onclick="moveCamera('right')">➡️ Right</button>
                </div>
            </div>
            
            <div class="panel control-panel">
                <h3>🎮 Movement Controls</h3>
                <div class="control-grid">
                    <button class="control-btn forward" onmousedown="startMoving('forward')" onmouseup="stopMoving()">⬆️</button>
                    <button class="control-btn left" onmousedown="startMoving('left')" onmouseup="stopMoving()">⬅️</button>
                    <button class="control-btn stop" onclick="stopMoving()">🛑</button>
                    <button class="control-btn right" onmousedown="startMoving('right')" onmouseup="stopMoving()">➡️</button>
                    <button class="control-btn backward" onmousedown="startMoving('backward')" onmouseup="stopMoving()">⬇️</button>
                </div>
                <div class="system-controls">
                    <button class="system-btn" onclick="toggleMotors()">🔧 Toggle Motors</button>
                    <button class="system-btn" onclick="toggleAutonomous()">🤖 Auto Mode</button>
                </div>
            </div>
            
            <div class="panel sensor-panel">
                <h3>📊 Sensor Data</h3>
                <div class="sensor-grid">
                    <div class="sensor-card">
                        <div class="sensor-value" id="temperature">--°C</div>
                        <div class="sensor-label">Temperature</div>
                    </div>
                    <div class="sensor-card">
                        <div class="sensor-value" id="humidity">--%</div>
                        <div class="sensor-label">Humidity</div>
                    </div>
                    <div class="sensor-card">
                        <div class="sensor-value" id="soilMoisture">--%</div>
                        <div class="sensor-label">Soil Moisture</div>
                    </div>
                    <div class="sensor-card">
                        <div class="sensor-value" id="distance">-- cm</div>
                        <div class="sensor-label">Distance</div>
                    </div>
                    <div class="sensor-card">
                        <div class="sensor-value" id="waterLevel">--%</div>
                        <div class="sensor-label">Water Level</div>
                    </div>
                </div>
                <div class="pump-controls">
                    <button class="pump-btn" id="pumpOnBtn" onclick="controlPump('on')">💧 Pump ON</button>
                    <button class="pump-btn" id="pumpOffBtn" onclick="controlPump('off')">⏹️ Pump OFF</button>
                </div>
            </div>
        </div>
        
        <div class="panel settings-panel">
            <h3>⚙️ System Settings</h3>
            <div class="threshold-controls">
                <div class="threshold-group">
                    <label>Obstacle Distance Threshold (cm)</label>
                    <input type="number" class="threshold-input" id="obstacleThreshold" placeholder="20">
                </div>
                <div class="threshold-group">
                    <label>Soil Moisture Threshold (%)</label>
                    <input type="number" class="threshold-input" id="soilThreshold" placeholder="30">
                </div>
                <div class="threshold-group">
                    <label>Water Level Threshold (%)</label>
                    <input type="number" class="threshold-input" id="waterThreshold" placeholder="20">
                </div>
                <div class="threshold-group">
                    <button class="system-btn" onclick="updateThresholds()">💾 Update Settings</button>
                </div>
            </div>
        </div>
    </div>
    
    <script>
        let movingInterval = null;
        let currentDirection = null;
        
        document.addEventListener('DOMContentLoaded', function() {
            initializeSystem();
            setInterval(updateStatus, 2000);
            setInterval(updateSensorData, 3000);
        });
        
        async function initializeSystem() {
            await updateStatus();
            await updateSensorData();
            await loadSettings();
            initializeCamera();
        }
        
        async function updateStatus() {
            try {
                const response = await fetch('/status');
                const status = await response.json();
                
                // Update connection status
                const connIndicator = document.getElementById('connectionIndicator');
                const connStatus = document.getElementById('connectionStatus');
                if (status.wifi_connected) {
                    connIndicator.className = 'status-indicator status-green';
                    connStatus.textContent = '🟢 WiFi Connected';
                } else {
                    connIndicator.className = 'status-indicator status-yellow';
                    connStatus.textContent = '🟡 Hotspot Mode';
                }
                
                // Update motor status
                const motorIndicator = document.getElementById('motorIndicator');
                const motorStatus = document.getElementById('motorStatus');
                if (status.motors_enabled) {
                    motorIndicator.className = 'status-indicator status-green';
                    motorStatus.textContent = 'Motors Enabled';
                } else {
                    motorIndicator.className = 'status-indicator status-red';
                    motorStatus.textContent = 'Motors Disabled';
                }
                
                // Update autonomous status
                const autoIndicator = document.getElementById('autonomousIndicator');
                const autoStatus = document.getElementById('autonomousStatus');
                if (status.autonomous_mode) {
                    autoIndicator.className = 'status-indicator status-green';
                    autoStatus.textContent = 'Autonomous Mode';
                } else {
                    autoIndicator.className = 'status-indicator status-yellow';
                    autoStatus.textContent = 'Manual Mode';
                }
                
                // Update pump status
                const pumpIndicator = document.getElementById('pumpIndicator');
                const pumpStatus = document.getElementById('pumpStatus');
                const pumpOnBtn = document.getElementById('pumpOnBtn');
                const pumpOffBtn = document.getElementById('pumpOffBtn');
                
                if (status.pump_active) {
                    pumpIndicator.className = 'status-indicator status-green';
                    pumpStatus.textContent = 'Pump Active';
                    pumpOnBtn.classList.add('pump-active');
                    pumpOffBtn.classList.remove('pump-active');
                } else {
                    pumpIndicator.className = 'status-indicator status-red';
                    pumpStatus.textContent = 'Pump Off';
                    pumpOnBtn.classList.remove('pump-active');
                    pumpOffBtn.classList.add('pump-active');
                }
                
                // Update camera stream URL
                if (status.cam_stream_url) {
                    document.getElementById('videoStream').src = status.cam_stream_url;
                }
                
            } catch (error) {
                console.error('Status update failed:', error);
                document.getElementById('connectionStatus').textContent = '🔴 Connection Error';
            }
        }
        
        async function updateSensorData() {
            try {
                const response = await fetch('/sensor-data');
                const data = await response.json();
                
                document.getElementById('temperature').textContent = data.temperature.toFixed(1) + '°C';
                document.getElementById('humidity').textContent = data.humidity.toFixed(1) + '%';
                document.getElementById('soilMoisture').textContent = data.soil_moisture + '%';
                document.getElementById('distance').textContent = data.distance.toFixed(1) + ' cm';
                document.getElementById('waterLevel').textContent = data.water_level + '%';
                
            } catch (error) {
                console.error('Sensor data update failed:', error);
            }
        }
        
        async function loadSettings() {
            try {
                const response = await fetch('/settings');
                const settings = await response.json();
                
                document.getElementById('obstacleThreshold').value = settings.obstacle_threshold;
                document.getElementById('soilThreshold').value = settings.soil_moisture_threshold;
                document.getElementById('waterThreshold').value = settings.water_level_threshold;
                
            } catch (error) {
                console.error('Settings load failed:', error);
            }
        }
        
        function initializeCamera() {
            const video = document.getElementById('videoStream');
            
            video.onload = function() {
                console.log('Camera connected');
            };
            
            video.onerror = function() {
                this.alt = 'Camera not available. Please ensure ESP32-CAM is connected.';
            };
        }
        
        function startMoving(direction) {
            if (currentDirection === direction) return;
            
            currentDirection = direction;
            sendCommand(direction);
            
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
        }
        
        async function sendCommand(command) {
            try {
                await fetch(`/${command}`);
            } catch (error) {
                console.error('Command failed:', error);
            }
        }
        
        async function toggleMotors() {
            try {
                await fetch('/toggle-motors');
                setTimeout(updateStatus, 100);
            } catch (error) {
                console.error('Toggle motors failed:', error);
            }
        }
        
        async function toggleAutonomous() {
            try {
                await fetch('/toggle-autonomous');
                setTimeout(updateStatus, 100);
            } catch (error) {
                console.error('Toggle autonomous failed:', error);
            }
        }
        
        async function controlPump(action) {
            try {
                await fetch(`/pump-${action}`);
                setTimeout(updateStatus, 100);
            } catch (error) {
                console.error('Pump control failed:', error);
            }
        }
        
        async function moveCamera(direction) {
            try {
                await fetch(`/servo-${direction}`);
            } catch (error) {
                console.error('Camera control failed:', error);
            }
        }
        
        async function updateThresholds() {
            try {
                const obstacle = document.getElementById('obstacleThreshold').value;
                const soil = document.getElementById('soilThreshold').value;
                const water = document.getElementById('waterThreshold').value;
                
                const params = new URLSearchParams({
                    obstacle: obstacle,
                    soil: soil,
                    water: water
                });
                
                await fetch('/update-thresholds', {
                    method: 'POST',
                    body: params
                });
                
                alert('Settings updated successfully!');
                
            } catch (error) {
                console.error('Settings update failed:', error);
                alert('Failed to update settings');
            }
        }
        
        // Prevent context menu and selection on mobile
        document.addEventListener('contextmenu', e => e.preventDefault());
        document.addEventListener('selectstart', e => e.preventDefault());
    </script>
</body>
</html>
)rawliteral";
} 