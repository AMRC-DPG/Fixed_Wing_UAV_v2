/*
Script from FixedWing V2 that was demo'd at DSEI.
No feedback from LSS but does move on command.
All commands hard coded to match Unity animation.
*/

#include <Arduino.h>
#include <LSS.h>
#include <SoftwareSerial.h>

SoftwareSerial mySoftSerial(8, 9);
// ID set to default LSS ID = 0
#define LSS_ID		(0)
#define LSS_BAUD	(LSS_DefaultBaud)
// Choose the proper serial port for your platform
#define LSS_SERIAL	(mySoftSerial)	// ex: Many Arduino boards
#define USE_TIMER_1     true

// These define's must be placed at the beginning before #include "AVR_Slow_PWM.h"
// _PWM_LOGLEVEL_ from 0 to 4
// Don't define _PWM_LOGLEVEL_ > 0. Only for special ISR debugging only. Can hang the system.
#define _PWM_LOGLEVEL_      0

#if (_PWM_LOGLEVEL_ > 3)
  #if USE_TIMER_1
    #warning Using Timer1
  #elif USE_TIMER_1
    #warning Using Timer3
  #endif
#endif

#define USING_MICROS_RESOLUTION       true    //false

// To be included only in main(), .ino with setup() to avoid `Multiple Definitions` Linker Error
#include "AVR_Slow_PWM.h"

// Don't change these numbers to make higher Timer freq. System can hang
#define HW_TIMER_INTERVAL_MS        0.1f
#define HW_TIMER_INTERVAL_FREQ      10000L

volatile uint32_t startMicros = 0;

// Init AVR_Slow_PWM, each can service 16 different ISR-based PWM channels
AVR_Slow_PWM ISR_PWM;

//////////////////////////////////////////////////////

void TimerHandler()
{
  ISR_PWM.run();
}

//////////////////////////////////////////////////////

#define USING_PWM_FREQUENCY     false //true

//////////////////////////////////////////////////////

// FAN PWM Variables
uint32_t PWM_Pin    = 11;
float PWM_Freq   = 61.08f;   //1.0f;
uint32_t PWM_Period = 1000000 / PWM_Freq; // You can assign any interval for any timer here,

// minimum duty cycle for fan to accept valid signal.
float PWM_DutyCycle = 6.75;
int incomingByte = 0;

// Channel number used to identify associated channel
int channelNum;

LSS myLSS = LSS(LSS_ID);

void setup()
{
  Serial.begin(115200);

  LSS::initBus(LSS_SERIAL, LSS_BAUD);

  pinMode(7,INPUT_PULLUP);


  while (!Serial);

  delay(2000);

  Serial.print(F("\nStarting ISR_Modify_PWM on "));
  Serial.println(BOARD_NAME);
  Serial.println(AVR_SLOW_PWM_VERSION);
  Serial.print(F("CPU Frequency = "));
  Serial.print(F_CPU / 1000000);
  Serial.println(F(" MHz"));

  // Timer0 is used for micros(), millis(), delay(), etc and can't be used
  // Select Timer 1-2 for UNO, 1-5 for MEGA, 1,3,4 for 16u4/32u4
  // Timer 2 is 8-bit timer, only for higher frequency
  // Timer 4 of 16u4 and 32u4 is 8/10-bit timer, only for higher frequency

#if USE_TIMER_1

  ITimer1.init();

  // Using ATmega328 used in UNO => 16MHz CPU clock ,

  if (ITimer1.attachInterrupt(HW_TIMER_INTERVAL_FREQ, TimerHandler))
  {
    Serial.print(F("Starting  ITimer1 OK, micros() = "));
    Serial.println(micros());
  }
  else
    Serial.println(F("Can't set ITimer1. Select another freq. or timer"));

#endif

  Serial.print(F("Using PWM Freq = "));
  Serial.print(PWM_Freq);
  Serial.print(F(", PWM DutyCycle = "));
  Serial.println(PWM_DutyCycle);

#if USING_PWM_FREQUENCY
  // You can use this with PWM_Freq in Hz
  ISR_PWM.setPWM(PWM_Pin, PWM_Freq1, PWM_DutyCycle);

#else
#if USING_MICROS_RESOLUTION
  // Or using period in microsecs resolution
  channelNum = ISR_PWM.setPWM_Period(PWM_Pin, PWM_Period, PWM_DutyCycle);
#else
  // Or using period in millisecs resolution
  channelNum = ISR_PWM.setPWM_Period(PWM_Pin, PWM_Period / 1000.0, PWM_DutyCycle);
#endif
#endif

  while (!Serial);

  delay(1000);

  myLSS.setMaxSpeed(1200, LSS_SetConfig);  //fast
  myLSS.setMotionControlEnabled(true);
  myLSS.setAngularAcceleration(600,LSS_SetConfig);
  myLSS.setAngularDeceleration(600,LSS_SetConfig);
  myLSS.setAngularStiffness(-2,LSS_SetConfig);
  myLSS.setAngularHoldingStiffness(2, LSS_SetConfig);
}

////////////////////////////////////////////////

void setFanPWM(float PWM_Duty) {
  PWM_DutyCycle = PWM_Duty;
  // You can use this with PWM_Freq in Hz
  if (!ISR_PWM.modifyPWMChannel(channelNum, PWM_Pin, PWM_Freq, PWM_DutyCycle))
  {
    Serial.print(F("modifyPWMChannel error for PWM_Period"));
  }
}

void printPWM(){
  Serial.print(F("Using PWM Freq = "));
  Serial.print(PWM_Freq);
  Serial.print(F(", PWM DutyCycle = "));
  Serial.println(PWM_DutyCycle);
}

void loop() {
  if (Serial.available() > 0) {

    incomingByte = Serial.parseFloat();

    Serial.print("I received: ");
    Serial.println(incomingByte, DEC);
    if (incomingByte <=9) {
      if (incomingByte == 9) {
        setFanPWM(6.75f);
      }
      else if (incomingByte == 8) {
        setFanPWM(7.5f);  //7.35 lowest for fan starting
      }
      else if (incomingByte == 2) {
        myLSS.moveT(0, 4400);
      }
      else if (incomingByte == 3) {
        myLSS.moveT(900, 2000);
      }
      else if (incomingByte == 4) {
        myLSS.moveT(0, 13400);
      }
      else if (incomingByte == 5) {
        myLSS.moveT(900, 1000);
      }
    }
  }
}