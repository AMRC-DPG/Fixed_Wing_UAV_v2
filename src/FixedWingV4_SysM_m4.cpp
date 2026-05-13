/* =================================================================================
* PROJECT: Fixed Wing Aerodynamic Demonstrator - Main Flight Controller
 * DESCRIPTION: Provides high-resolution hardware PWM control for an EDF,
 * dynamic safety factors, and non-blocking JSON telemetry output
 * for a Lynxmotion Smart Servo (LSS) via serial interface.
 * =================================================================================
 * COMMAND PROTOCOLS (Sent via PC Serial @ 115200 Baud, LSS at 38400 Baud):
 * - Fan Control:    "F:<PWM>"       (e.g., "F:1200" sets fan to 1200us)
 * - Servo Control:  "S:<DEG>:<MS>"  (e.g., "S:45.5:1000" moves to 45.5° in 1 sec)
 * - On-the-Fly Cfg: "C:<TYP>:<VAL>" (e.g., "C:AS:2" sets Angular Stiffness to 2)
 * - Emergency Stop: "STOP"          (Disarms fan, sets servo to limp)
 * =================================================================================
 */

#include <Arduino.h>
#include <ArduinoJson.h>    //Feedback data packets in JSON
#include <Servo.h>

// --- FUNCTION PROTOTYPES ---
void setFanMicroseconds(uint16_t microseconds);
float estimateFanRPM(uint16_t microseconds);
void readLSSResponsesFast();
void parseLSSResponseFast();
void sendLiveJSON();
void sendCfgJSON();
void processIncomingCommand(const String& cmd);
void handleFanCommand(const String& cmd);
void handleServoCommand(const String& cmd);
void handleConfigCommand(const String& cmd);
void handleEmergencyStop();

bool routine = false;
bool horiz = false;

// --- SERIAL SETUP ---
#define LSS_BAUD      (38400)
#define LSS_SERIAL    Serial1
// --- HARDWARE PWM SETUP (TIMER 1 @ 50Hz, 0.5us resolution) ---
const uint32_t PWM_Pin = 10;   // MUST be Pin 10 for Timer 1 (OC1B)

Servo fanESC; //switch to servo library object for fan


// --- FAN SAFETY & ESTIMATION CONSTANTS ---
const uint16_t ESC_ARM       = 1100; // 0% Throttle (1.1ms) - Arming
const uint16_t FAN_MIN_SUSTAIN  = 1125; // Lowest speed Fan can maintain once spinning
const uint16_t FAN_MIN_START    = 1136; // Minimum to break stiction
const uint16_t ESC_TEST      = 1160; // Low test speed (1.2ms) ~ 0.86A
//Increase this to enable faster speeds but stay below 10A current draw on power supply
const uint16_t LAB_SAFE_MAX  = 1300; // LAB SAFE LIMIT (1.3ms) ~ 4.82A - 1300 safe from ceiling tile movement
const uint16_t ESC_MAX       = 2000; // 100% Throttle (2.0ms) - Never run at this! as the power supply max is 10A
const float MAX_LOADED_RPM   = 28000.0f; //max RPM val at 2ms

// --- FAN LIVE TELEMETRY VARIABLES ---
uint16_t currentFanPWM = ESC_ARM; // Variable to track fan PWM value

// --- FAST FEEDBACK VARIABLES ---
const byte MAX_BUFFER = 48; // Increased slightly for longer String replies
char inputBuffer[MAX_BUFFER];
byte bufferIndex = 0;
int incomingByte = 0;


// --- LIVE TELEMETRY VARIABLES ---
float servoPosition = 0.0;
int16_t servoRPM = 0;
uint16_t servoCurrent = 0;
uint16_t servoVoltage = 0;
int16_t servoTemp = 0;
int8_t servoStatus = 0; // 6=Holding, 5=Moving, 1=Limp, etc.

// --- STATIC CONFIGURATION VARIABLES ---
int16_t servoStiffness = 0;
int16_t servoHoldingStiffness = 0;
int32_t servoAccel = 0;
int32_t servoDecel = 0;
int16_t servoMaxSpeed = 0;
int8_t  servoLED = 0;
int8_t  servoGyre = 0;
int16_t servoRange = 0;
String  servoModel = "Unknown";
String  servoFirmware = "Unknown";

/* =================================================================================
 * SETUP FUNCTION
 * Initializes serial buses, configures 16-bit Timer1 for hardware PWM,
 * arms the ESC, and pushes default baseline configurations to the LSS.
 * ================================================================================= */
void setup() {
  // Setup PC Serial via standard USB
  Serial.begin(115200);

  // Setup LSS on Serial1
  LSS_SERIAL.begin(LSS_BAUD);

  delay(100);

  Serial.println(F("\n--- SYSTEM BOOTING ---"));
  Serial.println(F("High-Res Hardware PWM: Active"));
  Serial.println(F("Safety Governor: Active (Max 1300 microseconds)"));

  // Configure Timer 1 for 50Hz Hardware PWM
  fanESC.attach(PWM_Pin, ESC_ARM - 100, LAB_SAFE_MAX);
  // Start the fan at arming/idle state
  setFanMicroseconds(ESC_ARM);
  delay(2000); // Give ESC time to hear the arming beeps
  Serial.println(F("{\"system\":\"System Initialised\"}"));
}

/* =================================================================================
 * HARDWARE CONTROL, FEEDBACK and Estimation FUNCTIONS
 * ================================================================================= */

// --- LOOP RPM ESTIMATOR ---
float estimateFanRPM(uint16_t microseconds) {
  if (microseconds <= ESC_ARM) return 0.0;
  // Linear scale: 1100 to 2000
  float throttleFraction = (float)(microseconds - ESC_ARM) / (float)(ESC_MAX - ESC_ARM);
  return throttleFraction * MAX_LOADED_RPM;
}
/* =================================================================================
 * JSON TELEMETRY FUNCTIONS
 * Packages current memory state into JSON strings
 * ================================================================================= */
void sendLiveJSON() {
  JsonDocument doc;
  doc["type"] = "live";

  // Nested assignment is automatic in v7!
  doc["fan"]["pwm"] = currentFanPWM;
  doc["fan"]["rpm"] = (long)estimateFanRPM(currentFanPWM);

  doc["servo"]["pos"]  = servoPosition;
  doc["servo"]["rpm"]  = servoRPM;
  doc["servo"]["cur"]  = servoCurrent;
  doc["servo"]["vol"]  = servoVoltage;
  doc["servo"]["tmp"]  = servoTemp;
  doc["servo"]["stat"] = servoStatus;

  serializeJson(doc, Serial);
  Serial.println();
}

void sendCfgJSON() {
  JsonDocument doc;
  doc["type"] = "cfg";

  doc["fan"]["min"] = FAN_MIN_SUSTAIN;
  doc["fan"]["max"] = LAB_SAFE_MAX;

  doc["servo"]["model"]  = servoModel;
  doc["servo"]["fw"]     = servoFirmware;
  doc["servo"]["stiff"]  = servoStiffness;
  doc["servo"]["hStiff"] = servoHoldingStiffness;
  doc["servo"]["acc"]    = servoAccel;
  doc["servo"]["dec"]    = servoDecel;
  doc["servo"]["maxSpd"] = servoMaxSpeed;
  doc["servo"]["led"]    = servoLED;
  doc["servo"]["gyre"]   = servoGyre;

  serializeJson(doc, Serial);
  Serial.println();
}

/* =================================================================================
 * COMMAND ROUTING & PARSING
 * Modularised logic for safely handling incoming serial strings from the PC
 * ================================================================================= */

void processIncomingCommand(const String& cmd) {
  Serial.println(cmd);
  if (cmd.startsWith("F:")) {
    handleFanCommand(cmd);
  }
  else if (cmd.startsWith("S:")) {
    handleServoCommand(cmd);
  }
  else if (cmd.startsWith("C:")) {
    handleConfigCommand(cmd);
  }
  else if (cmd == "STOP") {
    handleEmergencyStop();
    routine = false;
  }
  else if (cmd == "LOOP") {
    routine = true;
    Serial.println(F("{\"info\":\"LOOP_ROUTINE_STARTED\"}"));
  }
  else {
    Serial.println(F("{\"warn\":\"UNKNOWN_COMMAND_FORMAT\"}"));
  }
}

void handleFanCommand(const String& cmd) {
  int colonIndex = cmd.indexOf(':');
  if (colonIndex > 0) {
    uint16_t targetPWM = cmd.substring(colonIndex + 1).toInt();
    setFanMicroseconds(targetPWM);
  }
}

void setFanMicroseconds(uint16_t microseconds) {
  if (microseconds > LAB_SAFE_MAX) microseconds = LAB_SAFE_MAX;
  if (microseconds < ESC_ARM) microseconds = ESC_ARM;

  // Kickstart logic
  if (currentFanPWM <= ESC_ARM && microseconds >= FAN_MIN_SUSTAIN) {
    fanESC.writeMicroseconds(1200);
    delay(350);
  }

  currentFanPWM = microseconds;
  fanESC.writeMicroseconds(currentFanPWM); // High-res M4 update
}

void handleServoCommand(const String& cmd) {
  int firstColon = cmd.indexOf(':');
  int secondColon = cmd.indexOf(':', firstColon + 1);

  if (firstColon > 0 && secondColon > 0) {
    float targetDegrees = cmd.substring(firstColon + 1, secondColon).toFloat();
    int targetTime = cmd.substring(secondColon + 1).toInt();
    long lssPosition = (long)(targetDegrees * 10.0);

    // --- 🛡SERVO SAFETY ---
    if (lssPosition > 900) {
      Serial.println(F("{\"warn\":\"SERVO_MAX_LIMIT\"}"));
      lssPosition = 900;
    } else if (lssPosition < 0) {
      Serial.println(F("{\"warn\":\"SERVO_MIN_LIMIT\"}"));
      lssPosition = 0;
    }

    LSS_SERIAL.print(F("#1D"));
    LSS_SERIAL.print(lssPosition);
    LSS_SERIAL.print(F("T"));
    LSS_SERIAL.print(targetTime);
    LSS_SERIAL.print(F("\r"));
  }
}

void handleConfigCommand(const String& cmd) {
  int firstColon = cmd.indexOf(':');
  int secondColon = cmd.indexOf(':', firstColon + 1);

  if (firstColon > 0 && secondColon > 0) {
    String configType = cmd.substring(firstColon + 1, secondColon);
    int newConfigVal = cmd.substring(secondColon + 1).toInt();

    if (configType == "AS" && newConfigVal >= -4 && newConfigVal <= 4) {
      LSS_SERIAL.print(F("#1AS")); LSS_SERIAL.print(newConfigVal); LSS_SERIAL.print(F("\r"));
      Serial.println(F("{\"info\":\"AS_COMMAND_SENT\"}"));
    }
    else if (configType == "AH" && newConfigVal >= -10 && newConfigVal <= 10) {
      LSS_SERIAL.print(F("#1AH")); LSS_SERIAL.print(newConfigVal); LSS_SERIAL.print(F("\r"));
      Serial.println(F("{\"info\":\"AH_COMMAND_SENT\"}"));
    }
    else if (configType == "AA" && newConfigVal >= 10 && newConfigVal <= 10000) {
      LSS_SERIAL.print(F("#1AA")); LSS_SERIAL.print(newConfigVal); LSS_SERIAL.print(F("\r"));
      Serial.println(F("{\"info\":\"AA_COMMAND_SENT\"}"));
    }
    else if (configType == "AD" && newConfigVal >= 10 && newConfigVal <= 10000) {
      LSS_SERIAL.print(F("#1AD")); LSS_SERIAL.print(newConfigVal); LSS_SERIAL.print(F("\r"));
      Serial.println(F("{\"info\":\"AD_COMMAND_SENT\"}"));
    }
    else {
      Serial.println(F("{\"warn\":\"INVALID_CONFIG_VAL_OR_TYPE\"}"));
    }
  }
}
// kill switch function
void handleEmergencyStop() {
  setFanMicroseconds(ESC_ARM);
  LSS_SERIAL.print(F("#1L\r"));
  Serial.println(F("{\"info\":\"EMERGENCY_STOP_ACTIVATED\"}"));
}

/* =================================================================================
 * MAIN LOOP
 * 1. Checks PC commands and processing
 * 2. 15ms sequential polling ring (Live data @ 66hz & Config data @ 1hz)
 * 3. Sweeps SoftwareSerial buffer for Servo replies
 * ================================================================================= */

void loop() {
  // CHECK FOR DYNAMIC PC COMMANDS
  if (Serial.available() > 0) {
    String commandString = Serial.readStringUntil('\n');
    commandString.trim();

    if (commandString.length() > 0) {
      processIncomingCommand(commandString); // Parse to correct processing function
    }
  }
  // LIVE TELEMETRY BATCH - 20ms loop 50Hz
  static unsigned long lastLiveBatch = 0;
  if (millis() - lastLiveBatch >= 20) {
    lastLiveBatch = millis();
    // Pos, Speed, Current, Voltage, Temp, Status
    LSS_SERIAL.print(F("#1QD\r#1QS\r#1QC\r#1QV\r#1QT\r#1Q\r"));
  }

  // FULL CONFIG BATCH (0.1Hz / Every 10 Seconds)
  static unsigned long lastCfgBatch = 0;
  if (millis() - lastCfgBatch >= 10000) {
    lastCfgBatch = millis();
    // Query EVERY config variable at once:
    // Stiffness, Holding, Accel, Decel, MaxSpeed, LED, Gyre, Range, Model, Firmware
    LSS_SERIAL.print(F("#1QAS\r#1QAH\r#1QAA\r#1QAD\r#1QSD\r#1QLED\r#1QG\r#1QAR\r#1QMS\r#1QF\r"));
  }

  // LISTEN FOR LSS RESPONSES
  readLSSResponsesFast();

  // --- AUTOMATED SERVO SWEEP ROUTINE ---
  static unsigned long lastRoutine = 0;

  if (routine) {
    // Triggers instantly on the first run (when lastRoutine is 0) or every 1000ms
    if (millis() - lastRoutine >= 1000 || lastRoutine == 0) {
      lastRoutine = millis();

      if (horiz) {
        handleServoCommand("S:0.0:500"); // Move back to 0
        horiz = false;
      }
      else {
        handleServoCommand("S:90.0:500"); // Move to 90 FIRST!
        horiz = true;
      }
    }
  }
  else {
    // When STOP is sent, reset the timer and orientation for the next run
    lastRoutine = 0;
    horiz = false;
  }
}
// --- FAST NON-BLOCKING SERIAL PARSING ---
void readLSSResponsesFast(){
  while (LSS_SERIAL.available()) {
    char inChar = (char)LSS_SERIAL.read();

    if (inChar == '\r') {
      inputBuffer[bufferIndex] = '\0';
      parseLSSResponseFast();
      bufferIndex = 0;
    }
    else if (bufferIndex < MAX_BUFFER - 1) {
      inputBuffer[bufferIndex] = inChar;
      bufferIndex++;
    }
  }
}

// Parses response from LSS queries for both live and config data
void parseLSSResponseFast() {
  // --- LIVE DATA ---
  if (strncmp(inputBuffer, "*1QD", 4) == 0)      servoPosition = atol(&inputBuffer[4]) / 10.0;
  else if (strncmp(inputBuffer, "*1QS", 4) == 0) servoRPM = atoi(&inputBuffer[4]) / 60;
  else if (strncmp(inputBuffer, "*1QC", 4) == 0) servoCurrent = atoi(&inputBuffer[4]);
  else if (strncmp(inputBuffer, "*1QV", 4) == 0) servoVoltage = atoi(&inputBuffer[4]);
  else if (strncmp(inputBuffer, "*1QT", 4) == 0) servoTemp = atoi(&inputBuffer[4]);
  else if (strncmp(inputBuffer, "*1Q", 3) == 0 && isDigit(inputBuffer[3])) {servoStatus = inputBuffer[3] - '0';
    sendLiveJSON();
  }

  // --- CONFIG DATA ---
  else if (strncmp(inputBuffer, "*1QAS", 5) == 0)  servoStiffness = atoi(&inputBuffer[5]);
  else if (strncmp(inputBuffer, "*1QAH", 5) == 0)  servoHoldingStiffness = atoi(&inputBuffer[5]);
  else if (strncmp(inputBuffer, "*1QAA", 5) == 0)  servoAccel = atol(&inputBuffer[5]);
  else if (strncmp(inputBuffer, "*1QAD", 5) == 0)  servoDecel = atol(&inputBuffer[5]);
  else if (strncmp(inputBuffer, "*1QSD", 5) == 0)  servoMaxSpeed = atoi(&inputBuffer[5]);
  else if (strncmp(inputBuffer, "*1QLED", 6) == 0) servoLED = atoi(&inputBuffer[6]);
  else if (strncmp(inputBuffer, "*1QG", 4) == 0)   servoGyre = atoi(&inputBuffer[4]);
  else if (strncmp(inputBuffer, "*1QAR", 5) == 0)  servoRange = atoi(&inputBuffer[5]);
  else if (strncmp(inputBuffer, "*1QMS", 5) == 0)  servoModel = String(&inputBuffer[5]);
  else if (strncmp(inputBuffer, "*1QF", 4) == 0) {
    servoFirmware = String(&inputBuffer[4]);
    sendCfgJSON();
  }
}