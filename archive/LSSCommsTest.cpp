/*
Script for testing comms to the LSS Smart servo, uses AltSoftSerial Library as SoftSerial wasn't functioning correctly and wasn't receiving feedback.
Found the comms issue was due to the servo not properly saving the Baudrate
*/

#include <Arduino.h>
#include <AltSoftSerial.h>

AltSoftSerial lss; // RX=8, TX=9 on Uno

void setup() {
    // --- 1. SILENCE THE FAN (PIN 3) ---
    pinMode(3, OUTPUT);
    // Push PWM frequency above human hearing (~31kHz)
    TCCR2A = _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);
    TCCR2B = _BV(WGM22) | _BV(CS22) | _BV(CS21) | _BV(CS20);
    OCR2A = 156;
    OCR2B = 17;

    // --- 2. INITIALIZE COMMUNICATIONS ---
    Serial.begin(115200); // High speed connection to your PC/Controller
    lss.begin(38400);     // The rock-solid servo connection

    // Send a startup message in JSON format
    Serial.println(F("{\"status\": \"system_ready\", \"servo_baud\": 38400}"));
}

void loop() {
    static unsigned long lastQueryTime = 0;
    const unsigned long queryInterval = 100; // Query 10 times per second (100ms)

    // --- 3. REQUEST TELEMETRY ---
    if (millis() - lastQueryTime > queryInterval) {
        lastQueryTime = millis();

        // Clear out any old junk in the buffer
        while(lss.available()) lss.read();

        // Directly query Servo ID #1 for its Position (QD)
        lss.print(F("#1QD\r"));
    }

    // --- 4. PARSE & FORMAT RESPONSE ---
    if (lss.available()) {
        // Read the incoming reply until the carriage return
        String response = lss.readStringUntil('\r');

        // Verify it is a valid position response from ID #1
        if (response.startsWith("*1QD")) {

            // Extract the number (skip the first 4 characters "*1QD")
            String valueStr = response.substring(4);

            // Convert to integer (e.g., "356")
            long rawValue = valueStr.toInt();

            // Lynxmotion reports in tenths of a degree. Divide by 10.0.
            float degrees = rawValue / 10.0;

            // --- 5. TRANSMIT JSON ---
            Serial.print(F("{\"servo_id\": 1, \"position_deg\": "));
            Serial.print(degrees, 1); // Print with exactly 1 decimal place
            Serial.println(F("}"));
        }
    }
}