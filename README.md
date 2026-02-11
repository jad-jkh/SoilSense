# SoilSense - Automated Irrigation Controller

Arduino-based irrigation system with soil moisture monitoring, automated pump control, and water level sensing.

## Features

- **Automated Watering**: Monitors soil moisture and waters plants automatically when dry
- **Smart Pump Control**: N-MOSFET driven DC pump with timed watering cycles
- **Water Level Monitoring**: Ultrasonic sensor prevents dry-run by detecting low water
- **Visual Feedback**: Multi-LED status indicators for system state
- **Safety Features**: Cooldown periods, low-water protection, error handling

## Hardware Components

### Required Parts
- Arduino Uno R3
- Capacitive soil moisture sensor (analog output)
- DC water pump (5-12V)
- N-Channel MOSFET (e.g., IRF540N, IRLZ44N)
- HC-SR04 Ultrasonic Distance Sensor
- 6x LEDs (red, 2x yellow, green, blue, white recommended)
  - OR: 3 separate LEDs + 1 common-cathode RGB LED for water level
- 6x 220Ω resistors (for LEDs)
- 10kΩ resistor (MOSFET pull-down)
- Diode (1N4007 or similar - flyback protection for pump)
- Breadboard and jumper wires
- Water reservoir/container
- Tubing for pump

## Pin Connections

### Soil Moisture Sensor
- VCC → Arduino 5V
- GND → Arduino GND
- AOUT → Arduino A0

### DC Pump (via N-MOSFET)
- MOSFET Gate → Arduino Pin 9
- MOSFET Source → GND
- MOSFET Drain → Pump negative terminal
- Pump positive → External power supply positive (5-12V)
- Power supply GND → Arduino GND (common ground)
- Flyback diode across pump terminals (cathode to +)
- 10kΩ resistor: Gate to GND (pull-down)

### HC-SR04 Ultrasonic Sensor
- VCC → Arduino 5V
- GND → Arduino GND
- TRIG → Arduino Pin 7
- ECHO → Arduino Pin 6

### LED Indicators
- **Watering LED** (Pin 10):
  - Anode → Pin 10
  - Cathode → 220Ω resistor → GND
  
- **Moisture OK LED** (Pin 11):
  - Anode → Pin 11
  - Cathode → 220Ω resistor → GND
  
- **Water Level RED LED** (Pin 12):
  - Anode → Pin 12
  - Cathode → 220Ω resistor → GND

- **Water Level YELLOW LED** (Pin 8):
  - Anode → Pin 8
  - Cathode → 220Ω resistor → GND

- **Water Level GREEN LED** (Pin 4):
  - Anode → Pin 4
  - Cathode → 220Ω resistor → GND
  
- **System OK LED** (Pin 13):
  - Anode → Pin 13
  - Cathode → 220Ω resistor → GND

**Note:** You can use either 3 separate LEDs (red, yellow, green) OR a common-cathode RGB LED for the water level indicator.

## Wiring Diagram (Text)

```
Arduino Uno R3
┌─────────────────┐
│                 │
│  A0 ←───────────┼─── Soil Moisture Sensor (AOUT)
│                 │
│  D6 ←───────────┼─── HC-SR04 (ECHO)
│  D7 ────────────┼──→ HC-SR04 (TRIG)
│                 │
│  D9 ────────────┼──→ MOSFET Gate (with 10kΩ pull-down to GND)
│                 │
│  D4 ────────────┼──→ LED (Water GREEN) ──220Ω── GND
│  D8 ────────────┼──→ LED (Water YELLOW) ──220Ω── GND
│  D10 ───────────┼──→ LED (Watering) ──220Ω── GND
│  D11 ───────────┼──→ LED (Moisture OK) ──220Ω── GND
│  D12 ───────────┼──→ LED (Water RED) ──220Ω── GND
│  D13 ───────────┼──→ LED (System OK) ──220Ω── GND
│                 │
│  5V ────────────┼──→ Sensors VCC
│  GND ───────────┼──→ Common Ground
│                 │
└─────────────────┘

N-MOSFET Circuit:
    External Power (+5-12V)
           │
           ├───→ Pump (+)
           │
      [Flyback Diode]
           │
    Pump (−) ───→ MOSFET Drain
                    │
              MOSFET Source ──→ GND
                    │
              Gate ←─── Arduino D9
                    │
              10kΩ ──→ GND
```

## Calibration

### 1. Soil Moisture Sensor Calibration

Run this test sketch to find your sensor's values:

```cpp
void setup() {
  Serial.begin(9600);
  pinMode(A0, INPUT);
}

void loop() {
  int value = analogRead(A0);
  Serial.print("Moisture: ");
  Serial.println(value);
  delay(1000);
}
```

**Calibration Steps:**
1. Place sensor in completely dry soil → note the value (e.g., 800)
2. Water the soil thoroughly → note the value (e.g., 300)
3. Update these constants in the main code:
   - `MOISTURE_THRESHOLD_DRY` = dry value - 50 (e.g., 750)
   - `MOISTURE_THRESHOLD_WET` = wet value + 50 (e.g., 350)

### 2. Water Level Sensor Calibration

1. Measure your reservoir depth in cm
2. Update `RESERVOIR_HEIGHT_CM` in the code
3. Set `LOW_WATER_THRESHOLD_CM` to minimum safe level (e.g., 5 cm)
4. Position ultrasonic sensor at the top of reservoir, facing down

## Configuration Parameters

Edit these in the code to match your setup:

```cpp
// Soil Moisture (adjust after calibration)
const int MOISTURE_THRESHOLD_DRY = 600;  // Your dry soil value
const int MOISTURE_THRESHOLD_WET = 400;  // Your wet soil value

// Reservoir Dimensions
const float RESERVOIR_HEIGHT_CM = 30.0;      // Your reservoir depth
const float LOW_WATER_THRESHOLD_CM = 5.0;    // Minimum safe water level

// Water Level LED Thresholds (as percentage)
const float WATER_LEVEL_RED = 20.0;      // Below 20% = Red
const float WATER_LEVEL_YELLOW = 50.0;   // 20-50% = Yellow
// Above 50% = Green

// Watering Behavior
const unsigned long WATERING_DURATION = 5000;     // Pump run time (5 seconds)
const unsigned long WATERING_COOLDOWN = 3600000;  // Time between waterings (1 hour)
const unsigned long SENSOR_READ_INTERVAL = 60000; // Check interval (1 minute)
```

## LED Status Indicators

| LED | Pin | Color (Suggested) | Meaning |
|-----|-----|-------------------|---------|
| System OK | 13 | White/Blue | Blinks every second when running normally |
| Moisture OK | 11 | Blue | ON when soil moisture is adequate |
| Watering | 10 | Yellow/Amber | ON when pump is running |
| Water RED | 12 | Red | ON when water level is 0-20% (Critical) |
| Water YELLOW | 8 | Yellow | ON when water level is 20-50% (Medium) |
| Water GREEN | 4 | Green | ON when water level is 50-100% (Good) |

**Water Level Indicator:** Only ONE of the three water level LEDs will be lit at a time, showing the current reservoir status.

## Operation

1. **Power On**: All LEDs blink 3 times during startup
2. **Normal Operation**: 
   - System LED blinks every second
   - Sensors read every 60 seconds
   - Status printed to Serial Monitor
3. **Automatic Watering**:
   - Triggers when soil is dry AND cooldown has elapsed AND water level is sufficient
   - Pump runs for configured duration (default 5 seconds)
   - Watering LED illuminates during pumping
4. **Safety Features**:
   - Won't water if reservoir is low
   - Enforces cooldown period between waterings
   - Prevents pump dry-run

## Serial Monitor Output

The Arduino sends real-time diagnostics and status updates via serial communication. This is extremely useful for debugging and monitoring your system.

### How to Access Serial Monitor:

1. **Connect Arduino to Computer**: Use USB cable
2. **Open Arduino IDE**
3. **Upload the Code**: Upload SoilSense.ino to your Arduino
4. **Open Serial Monitor**: 
   - Click `Tools` → `Serial Monitor`
   - OR press `Ctrl+Shift+M` (Windows/Linux) or `Cmd+Shift+M` (Mac)
5. **Set Baud Rate**: Select `9600 baud` from the dropdown in bottom-right corner

### What You'll See:

```
SoilSense Irrigation Controller Starting...
System Ready!
Soil Moisture Raw: 523
Water Level: 18.5 cm
Water Level: GREEN (Good)
===== SoilSense Status =====
Soil Moisture: 523 (OK)
Water Level: 18.5 cm (61%)
Pump Status: OFF
============================

Soil is dry - initiating watering
Starting pump...
Stopping pump
```

### Why Use Serial Monitor:

- **Real-time Debugging**: See exactly what your sensors are reading
- **Calibration**: Monitor raw sensor values to set proper thresholds
- **System Events**: Track when watering occurs, alerts trigger, etc.
- **Troubleshooting**: Identify issues with sensors or logic

**Note:** Serial Monitor only works when Arduino is connected to your computer via USB. Once deployed, the system runs independently without serial output.

## Troubleshooting

**Pump doesn't run:**
- Check MOSFET wiring and power supply
- Verify common ground between Arduino and pump power
- Test pump directly with power supply
- Check if low water alert is active

**Incorrect soil readings:**
- Ensure sensor is inserted into soil (not air)
- Check sensor wiring
- Recalibrate thresholds

**Water level always shows 0 or error:**
- Check ultrasonic sensor wiring
- Ensure sensor faces water surface directly
- Verify RESERVOIR_HEIGHT_CM matches your setup
- Check for obstacles blocking ultrasonic path

**Constant watering:**
- Increase MOISTURE_THRESHOLD_DRY value
- Verify sensor is in soil, not water
- Check cooldown period is appropriate

## Safety Notes

- **Electrical Safety**: Keep Arduino and connections away from water
- **Power Supply**: Use appropriate voltage for your pump (typically 5-12V)
- **MOSFET Selection**: Ensure MOSFET can handle your pump current
- **Flyback Diode**: Always include diode across pump to protect MOSFET
- **Testing**: Test system with small water volumes before deploying

## License

Personal project - free to use and modify.
