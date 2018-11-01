/*
  Modbus-Arduino Example - TempSensor (Modbus Serial)
  Copyright by André Sarmento Barbosa
  http://github.com/andresarmento/modbus-arduino
*/
#include "rtd.h"
#include <Modbus.h>
#include <ModbusSerial.h>

// Modbus Slave ID (1-247)
byte SENSOR_ID = 24;
byte ch;

// Modbus Registers Offsets (0-9999)
const word SENSOR_IREG[2] = { 0, 1 };

// Used Pins
const byte dePin = 4;
const byte ssPin = 8;
const byte ledPin = LED_BUILTIN;

// ModbusSerial object
ModbusSerial mb;
Rtd rtd;

long ts;

void setup() {
  int ret;

  pinMode (ledPin, OUTPUT);

  // Config Modbus Serial (port, speed, byte format)
  mb.config (&Serial1, 19200, MB_PARITY_EVEN, dePin);
  // Set the Slave ID (1-247)
  mb.setSlaveId (SENSOR_ID);

  // Add SENSOR_IREG register - Use addIreg() for analog Inputs
  mb.addIreg (SENSOR_IREG[0]);
  mb.addIreg (SENSOR_IREG[1]);

  // Initializes the RTD device, the pin /CS is pin 10 (/SS)
  ret = rtd.begin (ssPin);

  ts = millis();
}

void loop() {
  // Call once inside loop() - all magic here
  mb.task();

  // Read each two seconds
  if (millis() > ts + 2000) {
    byte c = ch & 1;
    
    digitalWrite (ledPin, 0);
    double r = rtd.temperature (c);
    word rw = lround (r * 100.0);
    mb.Ireg (SENSOR_IREG[c], rw);
    digitalWrite (ledPin, 1);
    ts = millis();
    ch++;
  }
}
