// ===== PIN DEFINITIONS =====
const int SOIL_MOISTURE_PIN = A0;

// PUMP CONTROL (N-MOSFET GATE)
const int PUMP_PIN = 9;

// Ultrasonic Sensor (HC-SR04)
const int TRIG_PIN = 7;
const int ECHO_PIN = 6;

// LED STATUS INDICATORS
const int LED_WATERING = 10;      // LED indicating watering in progress
const int LED_MOISTURE_OK = 11;   // LED indicating soil moisture is avg
const int LED_SYSTEM_OK = 13;     // LED indicating system is operational

// WATER LEVEL RGB LED
const int LED_WATER_RED = 12;     // RED = LOW
const int LED_WATER_YELLOW = 8;   // YELLOW = Medium
const int LED_WATER_GREEN = 4;    // GREEN = GOOD

  // CONFIGURATION PARAMETERS
// Soil Moisture Thresholds (adjust based on your sensor calibration)
const int MOISTURE_THRESHOLD_DRY = 600;    // Below this = soil is dry, needs water
const int MOISTURE_THRESHOLD_WET = 400;    // Below this = soil is moist enough

// Water Level Configuration
const float RESERVOIR_HEIGHT_CM = 30.0;     // Total reservoir depth in cm
const float LOW_WATER_THRESHOLD_CM = 5.0;   // Water level below this triggers alert

// Water Level LED Thresholds (as percentage of reservoir height)
const float WATER_LEVEL_RED = 20.0;      // Below 20% = Red (critical)
const float WATER_LEVEL_YELLOW = 50.0;   // 20-50% = Yellow (medium)
// Above 50% = Green (good)

// Watering Parameters
const unsigned long WATERING_DURATION = 5000;      // Pump runs for 5 seconds
const unsigned long WATERING_COOLDOWN = 3600000;   // 1 hour between watering cycles (3,600,000 ms)
const unsigned long SENSOR_READ_INTERVAL = 60000;  // Check sensors every 60 seconds

// ===== GLOBAL VARIABLES =====
unsigned long lastWateringTime = 0;
unsigned long lastSensorReadTime = 0;
bool isPumpRunning = false;
unsigned long pumpStartTime = 0;

int soilMoistureValue = 0;
float waterLevelCm = 0;
bool lowWaterAlert = false;

// ===== SETUP =====
void setup() {
  // Initialize serial communication
  Serial.begin(9600);
  Serial.println("SoilSense Irrigation Controller Starting...");
  
  // Configure pins
  pinMode(SOIL_MOISTURE_PIN, INPUT);
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  pinMode(LED_WATERING, OUTPUT);
  pinMode(LED_MOISTURE_OK, OUTPUT);
  pinMode(LED_WATER_RED, OUTPUT);
  pinMode(LED_WATER_YELLOW, OUTPUT);
  pinMode(LED_WATER_GREEN, OUTPUT);
  pinMode(LED_SYSTEM_OK, OUTPUT);
  
  // Ensure pump is off at startup
  digitalWrite(PUMP_PIN, LOW);
  
  // Initial LED states
  digitalWrite(LED_WATERING, LOW);
  digitalWrite(LED_MOISTURE_OK, HIGH);
  digitalWrite(LED_WATER_RED, LOW);
  digitalWrite(LED_WATER_YELLOW, LOW);
  digitalWrite(LED_WATER_GREEN, LOW);
  digitalWrite(LED_SYSTEM_OK, HIGH);
  
  // Startup blink sequence
  startupBlink();
  
  Serial.println("System Ready!");
}

// ===== MAIN LOOP =====
void loop() {
  unsigned long currentTime = millis();
  
  // Update LED indicators
  updateSystemLED();
  
  // Handle pump timing if currently running
  if (isPumpRunning) {
    if (currentTime - pumpStartTime >= WATERING_DURATION) {
      stopPump();
    }
  }
  
  // Periodic sensor reading
  if (currentTime - lastSensorReadTime >= SENSOR_READ_INTERVAL) {
    lastSensorReadTime = currentTime;
    
    // Read sensors
    soilMoistureValue = readSoilMoisture();
    waterLevelCm = readWaterLevel();
    
    // Check water level
    checkWaterLevel();
    
    // Update moisture status LED
    updateMoistureLED();
    
    // Print status to serial
    printStatus();
    
    // Decide if watering is needed
    if (shouldWater(currentTime)) {
      startPump();
    }
  }
  
  delay(100); // Small delay to prevent excessive looping
}

// ===== SENSOR READING FUNCTIONS =====

int readSoilMoisture() {
  int rawValue = analogRead(SOIL_MOISTURE_PIN);
  Serial.print("Soil Moisture Raw: ");
  Serial.println(rawValue);
  return rawValue;
}

float readWaterLevel() {
  // Send ultrasonic pulse
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // Read echo pulse duration
  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms timeout
  
  if (duration == 0) {
    Serial.println("Ultrasonic sensor error - no echo received");
    return -1; // Error reading
  }
  
  // Calculate distance (speed of sound = 343 m/s or 0.0343 cm/µs)
  float distanceCm = (duration * 0.0343) / 2.0;
  
  // Calculate water level (reservoir height - distance to water surface)
  float waterLevel = RESERVOIR_HEIGHT_CM - distanceCm;
  
  // Ensure valid range
  if (waterLevel < 0) waterLevel = 0;
  if (waterLevel > RESERVOIR_HEIGHT_CM) waterLevel = RESERVOIR_HEIGHT_CM;
  
  Serial.print("Water Level: ");
  Serial.print(waterLevel);
  Serial.println(" cm");
  
  return waterLevel;
}

// ===== DECISION LOGIC =====

bool shouldWater(unsigned long currentTime) {
  // Don't water if pump is already running
  if (isPumpRunning) {
    return false;
  }
  
  // Don't water if in cooldown period
  if (currentTime - lastWateringTime < WATERING_COOLDOWN) {
    return false;
  }
  
  // Don't water if water level is too low
  if (lowWaterAlert) {
    Serial.println("Cannot water - reservoir level too low!");
    return false;
  }
  
  // Water if soil is dry
  if (soilMoistureValue >= MOISTURE_THRESHOLD_DRY) {
    Serial.println("Soil is dry - initiating watering");
    return true;
  }
  
  return false;
}

void checkWaterLevel() {
  // Turn off all water level LEDs first
  digitalWrite(LED_WATER_RED, LOW);
  digitalWrite(LED_WATER_YELLOW, LOW);
  digitalWrite(LED_WATER_GREEN, LOW);
  
  // Calculate water level as percentage of reservoir height
  float waterPercentage = (waterLevelCm / RESERVOIR_HEIGHT_CM) * 100.0;
  
  // Set low water alert for pump protection
  if (waterLevelCm <= LOW_WATER_THRESHOLD_CM && waterLevelCm >= 0) {
    lowWaterAlert = true;
    Serial.println("WARNING: Water level CRITICAL - pump disabled!");
  } else {
    lowWaterAlert = false;
  }
  
  // Update water level LEDs based on percentage
  if (waterPercentage <= WATER_LEVEL_RED) {
    // Critical low - RED
    digitalWrite(LED_WATER_RED, HIGH);
    Serial.println("Water Level: RED (Critical)");
  } 
  else if (waterPercentage <= WATER_LEVEL_YELLOW) {
    // Medium - YELLOW
    digitalWrite(LED_WATER_YELLOW, HIGH);
    Serial.println("Water Level: YELLOW (Medium)");
  } 
  else {
    // Good - GREEN
    digitalWrite(LED_WATER_GREEN, HIGH);
    Serial.println("Water Level: GREEN (Good)");
  }
}

void updateMoistureLED() {
  if (soilMoistureValue < MOISTURE_THRESHOLD_WET) {
    digitalWrite(LED_MOISTURE_OK, HIGH); // Soil is wet
  } else {
    digitalWrite(LED_MOISTURE_OK, LOW);  // Soil is dry
  }
}

// ===== PUMP CONTROL =====

void startPump() {
  if (lowWaterAlert) {
    Serial.println("Cannot start pump - water level too low!");
    return;
  }
  
  Serial.println("Starting pump...");
  digitalWrite(PUMP_PIN, HIGH);
  digitalWrite(LED_WATERING, HIGH);
  isPumpRunning = true;
  pumpStartTime = millis();
  lastWateringTime = millis();
}

void stopPump() {
  Serial.println("Stopping pump");
  digitalWrite(PUMP_PIN, LOW);
  digitalWrite(LED_WATERING, LOW);
  isPumpRunning = false;
}

// ===== LED FUNCTIONS =====

void updateSystemLED() {
  // Blink system LED to show Arduino is running
  static unsigned long lastBlink = 0;
  static bool ledState = false;
  
  if (millis() - lastBlink >= 1000) {
    lastBlink = millis();
    ledState = !ledState;
    digitalWrite(LED_SYSTEM_OK, ledState);
  }
}

void startupBlink() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_WATERING, HIGH);
    digitalWrite(LED_MOISTURE_OK, HIGH);
    digitalWrite(LED_WATER_RED, HIGH);
    digitalWrite(LED_WATER_YELLOW, HIGH);
    digitalWrite(LED_WATER_GREEN, HIGH);
    digitalWrite(LED_SYSTEM_OK, HIGH);
    delay(200);
    
    digitalWrite(LED_WATERING, LOW);
    digitalWrite(LED_MOISTURE_OK, LOW);
    digitalWrite(LED_WATER_RED, LOW);
    digitalWrite(LED_WATER_YELLOW, LOW);
    digitalWrite(LED_WATER_GREEN, LOW);
    digitalWrite(LED_SYSTEM_OK, LOW);
    delay(200);
  }
}

// ===== STATUS REPORTING =====

void printStatus() {
  Serial.println("===== SoilSense Status =====");
  Serial.print("Soil Moisture: ");
  Serial.print(soilMoistureValue);
  Serial.print(" (");
  if (soilMoistureValue < MOISTURE_THRESHOLD_WET) {
    Serial.print("WET");
  } else if (soilMoistureValue < MOISTURE_THRESHOLD_DRY) {
    Serial.print("OK");
  } else {
    Serial.print("DRY");
  }
  Serial.println(")");
  
  Serial.print("Water Level: ");
  Serial.print(waterLevelCm);
  Serial.print(" cm (");
  Serial.print((waterLevelCm / RESERVOIR_HEIGHT_CM) * 100.0);
  Serial.println("%)");
  
  Serial.print("Pump Status: ");
  Serial.println(isPumpRunning ? "RUNNING" : "OFF");
  
  if (lowWaterAlert) {
    Serial.println("ALERT: LOW WATER!");
  }
  
  Serial.println("============================");
  Serial.println();
}
