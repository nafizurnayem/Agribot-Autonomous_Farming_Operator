# 🛠️ AgroBot - Bill of Materials & Assembly Guide

## 📋 Complete Bill of Materials

### Core Electronics

| Component | Specifications | Quantity | Estimated Cost (USD) | Purpose |
|-----------|---------------|----------|---------------------|---------|
| **ESP32 DevKit v1** | 30-pin, Wi-Fi + Bluetooth | 1 | $8 | Main controller |
| **ESP32-CAM** | With OV2640 camera | 1 | $10 | Video streaming |
| **L298N Motor Driver** | Dual H-Bridge, 2A per channel | 1 | $5 | Motor control |
| **12V DC Gear Motors** | 100-300 RPM, with encoder | 2 | $30 | Robot movement |
| **DHT11 Sensor** | Temperature & Humidity | 1 | $3 | Environmental monitoring |
| **Soil Moisture Sensor** | Analog output | 1 | $5 | Soil analysis |
| **HC-SR04 Ultrasonic** | Distance sensor | 1 | $3 | Obstacle detection |
| **Water Level Sensor** | Float switch or analog | 1 | $8 | Tank monitoring |
| **5V Water Pump** | Submersible, 3-6V DC | 1 | $12 | Irrigation system |
| **Relay Module** | 5V, 10A capacity | 1 | $3 | Pump control |
| **SG90 Servo Motor** | 180° rotation | 1 | $5 | Camera positioning |

### Power System

| Component | Specifications | Quantity | Estimated Cost (USD) | Purpose |
|-----------|---------------|----------|---------------------|---------|
| **Li-Po Battery 3.7V** | 2500mAh, 18650 cells | 12 | $60 | Main power source |
| **BMS Module** | 3S4P configuration | 1 | $15 | Battery protection |
| **Buck Converter** | 12V to 5V, 3A | 1 | $8 | 5V rail |
| **Buck Converter** | 12V to 3.3V, 2A | 1 | $6 | 3.3V rail |
| **Battery Holder** | 18650 x 12 cells | 1 | $10 | Battery mounting |
| **Charging Port** | DC Jack 12V | 1 | $3 | External charging |
| **Power Switch** | Toggle switch 10A | 1 | $2 | Main power control |

### Mechanical Components

| Component | Specifications | Quantity | Estimated Cost (USD) | Purpose |
|-----------|---------------|----------|---------------------|---------|
| **Robot Chassis** | Stainless steel frame | 1 | $25 | Main structure |
| **Robot Wheels** | 65mm diameter, rubber | 2 | $10 | Movement |
| **Water Tank** | 1-2 Liter, clear plastic | 1 | $15 | Water storage |
| **Mounting Brackets** | L-shaped, various sizes | 5 | $10 | Component mounting |
| **Waterproof Enclosure** | For electronics | 2 | $20 | Protection |
| **Flexible Tubing** | 6mm inner diameter | 2m | $8 | Water delivery |
| **Spray Nozzles** | Adjustable pattern | 2 | $6 | Water distribution |
| **Caster Wheel** | Support wheel (optional) | 1 | $5 | Stability |

### Electronic Components & Accessories

| Component | Specifications | Quantity | Estimated Cost (USD) | Purpose |
|-----------|---------------|----------|---------------------|---------|
| **Jumper Wires** | Male-Female, 20cm | 40 | $8 | Connections |
| **PCB Prototype Board** | 9x15cm | 2 | $10 | Custom circuits |
| **Resistors** | 10kΩ, 1kΩ, 470Ω | 20 | $5 | Pull-ups, LEDs |
| **LEDs** | 3mm, Red/Green/Blue | 6 | $3 | Status indicators |
| **Capacitors** | 1000µF, 100µF | 4 | $5 | Power filtering |
| **Heat Shrink Tubing** | Various sizes | 1m | $3 | Wire protection |
| **Screws & Bolts** | M3, M4 assorted | 50 | $8 | Assembly |
| **Cable Ties** | Various lengths | 20 | $3 | Wire management |
| **Velcro Strips** | Self-adhesive | 1m | $5 | Removable mounting |

### Tools Required

| Tool | Purpose |
|------|---------|
| **Soldering Iron & Solder** | Electronic connections |
| **Wire Strippers** | Wire preparation |
| **Screwdrivers** | Assembly |
| **Drill & Bits** | Mounting holes |
| **Multimeter** | Circuit testing |
| **Hot Glue Gun** | Secure mounting |
| **3D Printer** (optional) | Custom brackets |

## 💰 Cost Analysis

| Category | Estimated Total |
|----------|----------------|
| **Electronics** | $95 |
| **Power System** | $104 |
| **Mechanical** | $99 |
| **Accessories** | $50 |
| **Tools** (one-time) | $60 |
| **Total Project Cost** | **$348** |
| **Total with Tools** | **$408** |

## 🔧 Assembly Instructions

### Phase 1: Power System Assembly

1. **Battery Pack Configuration**
   ```
   Series Groups: [Cell1-Cell4] [Cell5-Cell8] [Cell9-Cell12]
   Voltage per group: 3.7V × 4 = 14.8V
   Total configuration: 3S4P (3 series, 4 parallel per group)
   ```

2. **BMS Installation**
   - Connect BMS balance leads to each group
   - Wire main power output through BMS
   - Test voltage and protection circuits

3. **Power Distribution**
   - Install buck converters for 5V and 3.3V rails
   - Add fuses for protection
   - Test all voltage levels under load

### Phase 2: Main Controller Setup

1. **ESP32 Preparation**
   - Flash the main agrobot code
   - Test basic functionality
   - Configure WiFi credentials

2. **Motor Driver Connection**
   ```
   ESP32 → L298N
   GPIO26 → IN1
   GPIO27 → IN2  
   GPIO14 → ENA
   GPIO32 → IN3
   GPIO33 → IN4
   GPIO25 → ENB
   12V → VCC
   GND → GND
   ```

3. **Motor Installation**
   - Mount motors to chassis
   - Connect to motor driver outputs
   - Test forward/backward/left/right movements

### Phase 3: Sensor Integration

1. **DHT11 Temperature/Humidity**
   ```
   DHT11 → ESP32
   VCC → 3.3V
   DATA → GPIO4
   GND → GND
   ```

2. **Soil Moisture Sensor**
   ```
   Sensor → ESP32
   VCC → 3.3V
   AOUT → GPIO35 (ADC1)
   GND → GND
   ```

3. **Ultrasonic Sensor (HC-SR04)**
   ```
   HC-SR04 → ESP32
   VCC → 5V
   TRIG → GPIO18
   ECHO → GPIO19
   GND → GND
   ```

4. **Water Level Sensor**
   ```
   Sensor → ESP32
   VCC → 3.3V
   OUT → GPIO34 (ADC1)
   GND → GND
   ```

### Phase 4: Camera System

1. **ESP32-CAM Setup**
   - Flash camera streaming code
   - Test camera functionality
   - Configure network settings

2. **Servo Motor (Camera Pan/Tilt)**
   ```
   SG90 → ESP32
   VCC → 5V
   PWM → GPIO23
   GND → GND
   ```

3. **Camera Mounting**
   - Create adjustable mount
   - Ensure clear field of view
   - Weatherproof if needed

### Phase 5: Water System

1. **Water Pump Installation**
   ```
   Relay → ESP32 & Pump
   ESP32 GPIO22 → Relay IN
   5V → Relay VCC
   12V+ → Relay COM
   Pump+ → Relay NO
   Pump- → 12V GND
   ```

2. **Tank & Distribution**
   - Mount water tank securely
   - Install level sensor
   - Connect tubing and nozzles
   - Test for leaks

### Phase 6: Chassis Assembly

1. **Main Structure**
   - Mount electronics in waterproof enclosures
   - Install battery pack with easy access
   - Ensure proper weight distribution

2. **Wheel & Motor Mounting**
   - Align wheels for straight movement
   - Secure motor mounts
   - Test movement on flat surface

3. **Cable Management**
   - Route all cables neatly
   - Use cable ties and protective tubing
   - Label connections for maintenance

## 🧪 Testing & Calibration

### Initial Testing
1. **Power System Test**
   - Verify all voltage levels
   - Check current consumption
   - Test battery runtime

2. **Communication Test**
   - ESP32 to ESP32-CAM connectivity
   - WiFi/Hotspot mode switching
   - Web interface functionality

3. **Sensor Calibration**
   - DHT11: Compare with reference
   - Soil moisture: Dry/wet calibration
   - Ultrasonic: Distance accuracy
   - Water level: Empty/full calibration

### Field Testing
1. **Movement Testing**
   - Forward/backward precision
   - Turning radius measurement
   - Obstacle avoidance verification

2. **Watering System**
   - Pump flow rate measurement
   - Coverage pattern testing
   - Water level monitoring

3. **Autonomous Operation**
   - Path following accuracy
   - Sensor-based decision making
   - Safety system verification

## 🔧 Maintenance Schedule

### Daily
- [ ] Check battery voltage
- [ ] Verify water level
- [ ] Test camera feed

### Weekly
- [ ] Clean sensors
- [ ] Check mechanical connections
- [ ] Test all movement functions

### Monthly
- [ ] Calibrate sensors
- [ ] Update software if needed
- [ ] Full system performance test

## 🚨 Safety Considerations

1. **Electrical Safety**
   - Use appropriate fuses
   - Waterproof all connections
   - Regular insulation checks

2. **Mechanical Safety**
   - Secure all moving parts
   - Emergency stop functionality
   - Safe operating procedures

3. **Chemical Safety**
   - Use appropriate water additives only
   - Regular system cleaning
   - Proper disposal of old components

## 🔄 Upgrade Paths

### Phase 3 Additions
- **GPS Module**: For precise positioning
- **pH Sensor**: Soil acidity monitoring
- **Light Sensor**: Day/night operation
- **Rain Sensor**: Weather adaptation

### Phase 4 Enhancements
- **Solar Panel**: Renewable energy
- **Larger Water Tank**: Extended operation
- **Additional Sensors**: NPK soil analysis
- **AI Camera**: Plant disease detection

## 📚 Research Enhancement Ideas

1. **Data Analytics**
   - Implement data logging to SD card
   - Create historical analysis graphs
   - Machine learning for pattern recognition

2. **Swarm Robotics**
   - Multiple robot coordination
   - Task distribution algorithms
   - Mesh networking communication

3. **Precision Agriculture**
   - Variable rate application
   - Crop health monitoring
   - Yield prediction models

4. **Sustainability Features**
   - Solar power integration
   - Water recycling system
   - Biodegradable component options

This comprehensive system provides an excellent foundation for agricultural robotics research while being practical and cost-effective to build. 