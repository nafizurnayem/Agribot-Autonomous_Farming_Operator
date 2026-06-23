# ⚡ AgroBot Quick Start Guide

## 🎯 Get Your Robot Running in 30 Minutes!

This guide helps you get the basic AgroBot (Phase 1) running quickly for initial testing and demonstration.

---

## 📦 What You Need to Start

### Minimum Components for Phase 1 Demo
- ✅ ESP32 DevKit (main controller)
- ✅ ESP32-CAM (video streaming)
- ✅ L298N Motor Driver
- ✅ 2x DC Motors (any voltage 6-12V)
- ✅ 12V Power Supply or Battery Pack
- ✅ Jumper wires
- ✅ Breadboard or PCB
- ✅ 2x Wheels
- ✅ Simple chassis (even cardboard works!)

### Software Requirements
- Arduino IDE (latest version)
- ESP32 board support package
- USB cables for programming

---

## ⚡ 15-Minute Hardware Setup

### Step 1: Power System (5 minutes)
```
Simple Power Setup:
12V Battery/Supply → L298N VCC
L298N 5V Out → ESP32 VIN (or use separate 5V adapter)
Common GND → All components
```

### Step 2: Motor Connections (5 minutes)
```
L298N → ESP32 Connections:
IN1 → GPIO 26
IN2 → GPIO 27
ENA → GPIO 14
IN3 → GPIO 32
IN4 → GPIO 33
ENB → GPIO 25

L298N Motor Outputs:
OUT1, OUT2 → Left Motor
OUT3, OUT4 → Right Motor
```

### Step 3: ESP32-CAM Setup (5 minutes)
```
ESP32-CAM Power:
5V → ESP32-CAM 5V pin
GND → ESP32-CAM GND

For Programming:
Use FTDI adapter or ESP32-CAM-MB programmer board
Connect GPIO0 to GND during upload, then disconnect
```

---

## 💻 15-Minute Software Setup

### Step 1: Arduino IDE Setup (5 minutes)
1. Install Arduino IDE from arduino.cc
2. Add ESP32 board support:
   - File → Preferences
   - Additional Boards Manager URLs: 
     `https://dl.espressif.com/dl/package_esp32_index.json`
   - Tools → Board → Boards Manager → Search "ESP32" → Install

### Step 2: Update Code (5 minutes)
1. Open `agrobot_main.ino`
2. **IMPORTANT**: Update WiFi credentials:
   ```cpp
   const char* wifi_ssid = "YOUR_WIFI_NAME";
   const char* wifi_password = "YOUR_WIFI_PASSWORD";
   ```

3. Open `esp32cam_stream.ino`
4. Update the same WiFi credentials

### Step 3: Upload Code (5 minutes)
1. **Upload to Main ESP32:**
   - Select Board: "ESP32 Dev Module"
   - Select correct COM port
   - Upload `agrobot_main.ino`

2. **Upload to ESP32-CAM:**
   - Connect GPIO0 to GND
   - Press reset button
   - Upload `esp32cam_stream.ino`
   - Disconnect GPIO0 from GND
   - Press reset button

---

## 🚀 First Test (5 minutes)

### Power On Sequence
1. Power on ESP32-CAM first
2. Power on main ESP32
3. Wait 30 seconds for network connection

### Check Status
- **ESP32 Serial Monitor**: Should show IP address
- **ESP32-CAM Serial Monitor**: Should show "Camera initialized"

### Access Web Interface
1. **Find IP Address**: Check serial monitor output
2. **Open Browser**: Go to `http://[ESP32_IP_ADDRESS]`
3. **Test Controls**: Try Forward/Backward/Left/Right buttons
4. **Check Video**: Should see live camera feed

---

## 🎮 Basic Operation

### Web Interface Controls
- **🎮 Movement**: Use directional buttons
- **🛑 Stop**: Red stop button (or release any direction)
- **📹 Video**: Should auto-load in top panel
- **⚙️ Motors**: Toggle to enable/disable movement

### Expected Behavior
- **Forward**: Both wheels rotate forward
- **Backward**: Both wheels rotate backward  
- **Left**: Left wheel backward, right wheel forward
- **Right**: Left wheel forward, right wheel backward
- **Stop**: All motors stop after 500ms automatically

---

## 🔧 Quick Troubleshooting

### 🚫 Robot Not Moving
```
Check List:
□ Motors getting 12V power?
□ L298N connections correct?
□ ESP32 programmed successfully?
□ Serial monitor showing commands?
□ Motors enabled in web interface?
```

### 📷 No Video Feed  
```
Check List:
□ ESP32-CAM powered with 5V?
□ Both devices on same network?
□ Camera initialization successful?
□ Correct IP address in browser?
□ Try accessing ESP32-CAM directly at [CAM_IP]:81
```

### 🌐 Can't Connect to Web Interface
```
Check List:
□ WiFi credentials correct?
□ Device on same network?
□ Try hotspot mode (network: "AgroBot_Hotspot", password: "agrobot123")
□ Check serial monitor for IP address
□ Try mDNS: http://agrobot.local
```

### ⚡ Power Issues
```
Check List:  
□ Battery charged?
□ All GND connections common?
□ 5V rail stable under load?
□ Motor driver getting adequate power?
□ Check for loose connections
```

---

## 🎯 Testing Checklist

### Phase 1 Functionality Test
- [ ] **Power On**: Both ESP32s boot successfully
- [ ] **Network**: Connects to WiFi or creates hotspot
- [ ] **Web Interface**: Loads without errors  
- [ ] **Video Stream**: Shows live camera feed
- [ ] **Forward Movement**: Both motors rotate correctly
- [ ] **Backward Movement**: Both motors reverse
- [ ] **Left Turn**: Differential motor control
- [ ] **Right Turn**: Differential motor control  
- [ ] **Stop Function**: Motors stop on command
- [ ] **Safety**: Auto-stop after 500ms
- [ ] **Status Display**: Shows connection and motor state

### Demo Scenarios
1. **Basic Movement**: Show all 4 directions + stop
2. **Video Quality**: Demonstrate real-time streaming
3. **Network Switching**: Show WiFi and hotspot modes
4. **Safety Features**: Demonstrate auto-stop
5. **Mobile Access**: Test on smartphone/tablet

---

## 🎉 Success! What's Next?

### Immediate Improvements
1. **Better Chassis**: Upgrade from cardboard to proper frame
2. **Battery Pack**: Add rechargeable battery system
3. **Wheels**: Get proper robot wheels for better traction
4. **Enclosures**: Protect electronics from dust/water

### Phase 2 Expansion (Add Sensors)
1. **DHT11**: Temperature and humidity monitoring
2. **Soil Moisture**: Add watering capability
3. **Ultrasonic**: Obstacle avoidance
4. **Water Pump**: Automatic irrigation
5. **Servo**: Camera pan/tilt control

### Documentation & Research
1. **Video Documentation**: Record operation for presentations
2. **Performance Metrics**: Measure speed, battery life, etc.
3. **Research Paper**: Start writing based on template provided
4. **Improvements Log**: Document all modifications and results

---

## 📞 Need Help?

### Common Resources
- **ESP32 Pinout**: Search "ESP32 pinout diagram"
- **L298N Guide**: Search "L298N motor driver tutorial"
- **ESP32-CAM Guide**: Search "ESP32-CAM camera streaming"

### Debug Tools
- **Serial Monitor**: Essential for troubleshooting
- **Multimeter**: Check voltages and continuity
- **Logic Analyzer**: For advanced debugging (optional)

### Community Support
- ESP32 Arduino Community Forums
- Reddit: r/esp32, r/arduino
- GitHub Issues on similar projects

---

## 🏆 Congratulations!

You now have a working agricultural robot with:
- ✅ Remote web control
- ✅ Live video streaming  
- ✅ Dual network capability
- ✅ Safety features
- ✅ Mobile-friendly interface
- ✅ Expandable architecture

**This is already a significant achievement!** Many commercial agricultural robots cost thousands of dollars and have similar basic functionality.

Your next step is to gradually add sensors and features from Phase 2, documenting everything for your research paper. Each addition will make your robot more capable and your research more valuable.

**Remember**: Start simple, test thoroughly, then expand. This iterative approach will give you the best results and most reliable system.

---

## 📋 Development Log Template

Keep track of your progress:

```
Date: ___________
Phase: 1 - Basic Movement and Video

Completed Today:
□ Hardware assembly
□ Software upload  
□ Basic testing
□ _______________

Issues Encountered:
- Issue 1: _______________
  Solution: ______________
  
- Issue 2: _______________
  Solution: ______________

Next Steps:
1. ____________________
2. ____________________
3. ____________________

Performance Notes:
- Battery life: _____ hours
- Video quality: _______
- Movement accuracy: ___
- Response time: _______
```

Good luck with your AgroBot project! 🚀🌱 