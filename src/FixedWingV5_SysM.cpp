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
#include <SoftwareSerial.h> //Comms to LSS

// --- SERIAL SETUP ---
// Ensure Switch in "XBee" mode, no jumpers on LSS sheild
SoftwareSerial lssSerial(8, 9); // RX = D8, TX = D9

#define LSS_BAUD      (38400)      // Better speed for LSS
#define LSS_SERIAL    (lssSerial)

// --- HARDWARE PWM SETUP (TIMER 1 @ 50Hz, 0.5us resolution) ---
#define PWM_Pin 10;   // MUST be Pin 10 for Timer 1 (OC1B)
#define TOP_VAL 39999; // Yields 50Hz with Prescaler 8 on 16MHz clock

// --- FAN SAFETY & ESTIMATION CONSTANTS ---
#define ESC_ARM 1100; // 0% Throttle (1.1ms) - Arming
#define FAN_MIN_SUSTAIN 1125; // Lowest speed Fan can maintain once spinning
#define FAN_MIN_START 1136; // Minimum to break stiction
#define ESC_TEST  1160; // Low test speed (1.2ms) ~ 0.86A
//Increase this to enable faster speeds but stay below 10A current draw on power supply
#define LAB_SAFE_MAX 1300; // LAB SAFE LIMIT (1.3ms) ~ 4.82A - 1300 safe from ceiling tile movement
#define ESC_MAX 2000; // 100% Throttle (2.0ms) - Never run at this! as the power supply max is 10A
#define MAX_LOADED_RPM  28000.0f; //max RPM val at 2ms

#define ID_OFFSET 24
long telemMessage;
//Limit to 16 bits data identifier, we should be good for a while
#define QD 0X1
#define QS 0x2
#define QC 0x3
#define QV 0x4
#define QT 0x5
#define QSS 0x6
#define QAS 0x7
#define QAH 0x8
#define QAA 0x9
#define QAD 0xA
#define QSD 0xB
#define QLED 0xC
#define QG 0xD
#define QMS 0xE
#define QF 0xF
#define QAR 0x10 //This isn't actually used

// --- FAN LIVE TELEMETRY VARIABLES ---
uint16_t currentFanPWM = ESC_ARM; // Variable to track fan PWM value

// --- FAST FEEDBACK VARIABLES ---

/* =================================================================================
 * SETUP FUNCTION
 * Initializes serial buses, configures 16-bit Timer1 for hardware PWM,
 * arms the ESC, and pushes default baseline configurations to the LSS.
 * ================================================================================= */
void setup() {
  // Setup PC Serial via standard USB
  Serial.begin(115200);

  // Setup LSS SoftwareSerial at the safe speed
  LSS_SERIAL.begin(LSS_BAUD);

  Serial.println(F("\n--- SYSTEM BOOTING ---"));
  Serial.println(F("High-Res Hardware PWM: Active"));
  Serial.println(F("Safety Governor: Active (Max 1300 microseconds)"));

  // Configure Timer 1 for 50Hz Hardware PWM
  pinMode(PWM_Pin, OUTPUT);
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  TCCR1A = _BV(COM1B1) | _BV(WGM11);
  TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS11);
  ICR1 = TOP_VAL;

  // Start the fan at arming/idle state
  setFanMicroseconds(ESC_ARM);
  delay(2000); // Give ESC time to hear the arming beeps

  // 4. Send initial configs to Servo ID 1
  LSS_SERIAL.print(F("#1SD1200\r")); // Max Speed
  LSS_SERIAL.print(F("#1EM1\r"));    // Motion Control Enable
  LSS_SERIAL.print(F("#1AA600\r"));  // Angular Accel
  LSS_SERIAL.print(F("#1AD600\r"));  // Angular Decel
  LSS_SERIAL.print(F("#1AS-2\r"));   // Angular Stiffness
  LSS_SERIAL.print(F("#1AH2\r"));    // Holding Stiffness

  Serial.println(F("System Armed and Ready."));
}

/* =================================================================================
 * COMMAND ROUTING & PARSING
 * Modularised logic for safely handling incoming serial strings from the PC
 * ================================================================================= */
void setFanMicroseconds(uint16_t microseconds) {
  // Safety Ceiling when in lab - update variable for higher speed tests
  if (microseconds > LAB_SAFE_MAX) {microseconds = LAB_SAFE_MAX;}

  // KICKSTART LOGIC - helps fan with sticktion at low speeds
  if (currentFanPWM <= ESC_ARM && microseconds >= FAN_MIN_SUSTAIN) {
    // Give it a 1200us "punch" regardless of the target to overcome sticktion
    OCR1B = 1200 * 2;
    delay(350); // 350ms is usually enough to clear sticktion
  }

  // If the request is in the sticktion & no spin zone
  if (microseconds > ESC_ARM && microseconds < FAN_MIN_SUSTAIN) {microseconds = ESC_ARM;}

  // if we are below the arming signal set ARM
  if (microseconds < ESC_ARM) {microseconds = ESC_ARM;}

  // Apply final value to Fan timer pin
  currentFanPWM = microseconds;
  OCR1B = currentFanPWM * 2;
}

void handleServoCommand(uint16_t value) {
 {
    int targetTime = (value&&0xFF00) >> 0x10; //Get the first half bytw between 9-16
    int lssPosition = (value&&0xFF)* 10; //Get the first 8 bits which covers 0 to 90

    // --- 🛡SERVO SAFETY ---
    if (lssPosition > 900) {
      //Serial.println(F("{\"warn\":\"SERVO_MAX_LIMIT\"}"));
      lssPosition = 900;
    } else if (lssPosition < 0) {
      //Serial.println(F("{\"warn\":\"SERVO_MIN_LIMIT\"}"));
      lssPosition = 0;
    }

    lssSerial.print(F("#1D"));
    lssSerial.print(lssPosition);
    lssSerial.print(F("T"));
    lssSerial.print(targetTime);
    lssSerial.print(F("\r"));
  }
}
// kill switch function
void handleEmergencyStop() {
  setFanMicroseconds(ESC_ARM);
  lssSerial.print(F("#1L\r"));
  Serial.println(F("{\"info\":\"EMERGENCY_STOP_ACTIVATED\"}"));
}

void processIncomingCommand(long command) {

  unsigned int type = ((command & 0xFFFF0000) >> 0x10); //pulls the first 16 bits which can be used as identifier 
  unsigned int value = ((command & 0xFFFF)) //Pulls the last 16 bits for values up to 65535

  switch(type)
  {
    case 1: //case 1 handles fans
      setFanMicroseconds(value);
      break;
    case 2: //case 2 handles servo
      handleServoCommand(value);
      break;
    case 3: //case 3 handles ESTOP
      handleEmergencyStop();
      break;
    case 4: //case 4 handles AS config
      if (value >= -4 && value <= 4)
      {
      lssSerial.print(F("#1AS")); lssSerial.print(value); lssSerial.print(F("\r"));
      }
      break;
    case 5: //case 5 handles AH Config
      if(value >= -10 && value <= 10)
      {
        lssSerial.print(F("#1AH")); lssSerial.print(value); lssSerial.print(F("\r"));
      }
      break;
    case 6: //case 6 handles AA config
      if (value >= 10 && value <= 10000)
      {
        lssSerial.print(F("#1AA")); lssSerial.print(value); lssSerial.print(F("\r"));
      }
      break;
    case 7: //case 7 handles AD config
      if (value >= 10 && _INLINE_VARIABLES_SUPPORTED <= 10000)
      {
        lssSerial.print(F("#1AD")); lssSerial.print(value); lssSerial.print(F("\r"));
      }
      break;

    default:
      break;
  }
}

/* =================================================================================
 * MAIN LOOP
 * 1. Checks PC commands and processing
 * 2. 15ms sequential polling ring (Live data @ 66hz & Config data @ 1hz)
 * 3. Sweeps SoftwareSerial buffer for Servo replies
 * ================================================================================= */

// --- FAST NON-BLOCKING SERIAL PARSING ---

void queryTelemetry (char** query, int typeMsg, int bufferOffset)
{
  #define MAX_BUFFER 48; // Increased slightly for longer String replies
  char inputBuffer[MAX_BUFFER];
  byte bufferIndex = 0;
  int incomingByte = 0;

  lssSerial.print(F(query));
  while (LSS_SERIAL.available())
  {
    char inChar = (char)LSS_SERIAL.read();

    if (inChar == '\r') {
      inputBuffer[bufferIndex] = '\0';
      bufferIndex = 0;
      break;
    }
    else if (bufferIndex < MAX_BUFFER - 1) {
      inputBuffer[bufferIndex] = inChar;
      bufferIndex++;
    }
  }
  
  if(typeMsg == QSS && isDigit(inputBuffer[3]))
  {
    long dataMsg = (typeMsg << ID_OFFSET) | (inputBuffer[bufferOffset] - '0');
  }
  else
  {
    long dataMsg = (typeMsg << ID_OFFSET) | atoi(&inputBuffer[bufferOffset]) 
  }
  
  Serial.println(dataMsg);
}

void loop() {
  // CHECK FOR DYNAMIC PC COMMANDS
  Serial.available() ? processIncomingCommand(Serial.read()); : continue; //This will only run if we have serial data available

  // TWO-TIER SEQUENCER (Fires every 15ms)
  static unsigned long lastQuery = 0;
  static int iteration = 0;

  if (millis() - lastQuery > 15)
  {
    lastQuery = millis();

    // --- THE FAST LOOP (Live Telemetry - 10Hz) ---
    switch(iteration)
    {
      case 0: queryTelemetry("#1QD\r",QD, 4); break;
      case 1: queryTelemetry("#1QS\r",QS, 4); break;
      case 2: queryTelemetry("#1QC\r",QC, 4); break;
      case 3: queryTelemetry("#1QV\r",QV, 4); break;
      case 4: queryTelemetry("#1QT\r",QT, 4); break;
      case 5: queryTelemetry("#1Q\r",QSS, 3); break;
      case 6:
      // --- THE SLOW LOOP (Static Configs - 1Hz) ---
        queryTelemetry("#1QAS\r", QAS, 5);
        queryTelemetry("#1QAH\r", QAH, 5);
        queryTelemetry("#1QAA\r", QAA, 5);
        queryTelemetry("#1QAD\r", QAD, 5);
        queryTelemetry("#1QSD\r", QSD, 5);
        queryTelemetry("#1QLED\r", QLED, 6);
        queryTelemetry("#1QG\r", QG, 4);
        queryTelemetry("#1QAR\r", QAR, 5);
        queryTelemetry("#1QMS\r", QMS, 5);
        queryTelemetry("#1QF\r", QF, 4);
      break;
    }
    iteration > 6 ? iteration = 0 : iteration++;
  }
}

