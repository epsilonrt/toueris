/*
 * TOUERIS MODBUS CTOR SLAVE
 */
#include <EEPROM.h>
#include <Modbus.h>
#include <ModbusSerial.h> // https://github.com/epsilonrt/modbus-arduino
#include <Hmi4DinBox.h>  // https://github.com/epsilonrt/WireHmi

//#include <Toueris2Hmi.h>  // https://github.com/epsilonrt/WireHmi
//#include <LCD_ST7032.h> // https://github.com/epsilonrt/LCD_ST7032
#include <PinChangeInterrupt.h> // https://github.com/NicoHood/PinChangeInterrupt
#include "bisrelay.h"
#include "version.h" // for update this file: $ git-version version.h 

// Modbus
const byte MODBUS_SLAVEID = 8;
const unsigned long MODBUS_BAUDRATE = 19200;
const byte MODBUS_CONFIG = MB_PARITY_EVEN;
const char MODBUS_STR_CONFIG[] = "8E1";
const char MODBUS_SLAVEID_DATA[] = "CTOR";

// Backlight
const byte eeBl = 0; // address in EEPROM
const unsigned long blTimeout = 30000; // 30 sec.

// Devices
const byte eeDev = 1; // address in EEPROM

// Used Pins
const int ledPin = LED_BUILTIN;
const int dePin = 4;
const int hirqPin = 7;
const int icomPin = 11;

// Global objects
ModbusSerial mb; // ModBus object
// Bistable relays
BisRelay rl[5] = {
  BisRelay (15, 16),
  BisRelay (14, 8),
  BisRelay (9, 10),
  BisRelay (5, 13),
  BisRelay (29, 25)
};
// I2c Human-Machine Interface
Hmi4DinBox hmi (hirqPin);
// ST7032 LCD
//LCD_ST7032 lcd;

// Global variables
volatile bool icomChange = false;
volatile bool icomState = false;
byte manRelay = 0;

byte dev;
bool devChange = false;

byte bl;
bool blChange = false;
bool blState  = true;
unsigned long blTime;

void icomHandler() {

  icomChange = true;
  icomState = digitalRead (icomPin);
}

void setup() {

  while (!hmi.begin(24)) { // start the HMI by checking that it has worked well...
    delay (500);
  }

  //          0123456789012345
  hmi.lcd.print ("CTOR v");
  hmi.lcd.print (VERSION_SHORT);
  hmi.lcd.setCursor (1, 0);
  hmi.lcd.print ("Waiting HMI...");

  dev = EEPROM.read (eeDev);
  if (dev & 0xE0) {
    dev = 0;
    EEPROM.write (eeDev, dev);
  }
  // hmi.lcd.print (dev, HEX);


  bl = EEPROM.read (eeBl) & 0xF8; // rounded to a multiple of 8
  hmi.backlight.write (bl);
  blTime = millis();

  // Config Modbus Serial (port, speed, byte format)
  mb.config (&Serial1, MODBUS_BAUDRATE, MODBUS_CONFIG, dePin);
  // Set the Slave ID (1-247)
  mb.setSlaveId (MODBUS_SLAVEID);
  mb.setAdditionalServerData (MODBUS_SLAVEID_DATA); // for Report Server ID function


  hmi.lcd.clear();
  //          0123456789012345
  hmi.lcd.print ("12345| CTOR @");
  hmi.lcd.print (MODBUS_SLAVEID);

  hmi.lcd.setCursor (1, 0);
  for (int i = 0; i < 5; i++) {

    mb.addCoil (i);

    rl[i].begin();
    if (dev & (1 << i)) {
      mb.Coil (i, true);
    }

    hmi.lcd.write (rl[i].state() ? 'X' : '-');
  }

  //          0123456789
  hmi.lcd.print ("| ");
  hmi.lcd.print (MODBUS_BAUDRATE);
  hmi.lcd.print (MODBUS_STR_CONFIG);

  attachPCINT (digitalPinToPCINT (icomPin), icomHandler, CHANGE);
  hmi.led.set (LED_GREEN1);
  hmi.lcd.cursor();
  hmi.lcd.blink();
}

void loop() {

  // Call once inside loop() - all magic here
  mb.task();

  // Icom Led
  if (icomChange) {
    if (icomState) {

      hmi.led.set (LED_YELLOW1);
    }
    else {

      hmi.led.clear (LED_YELLOW1);
    }
    icomChange = false;
  }

  if (hmi.keyb.available()) { // check if keys are available

    blTime = millis();
    if (!blState) {

      hmi.backlight.write (bl);
      blState = true;
    }

    if (hmi.keyb.pressed()) {

      switch (hmi.keyb.key()) {
        case KUP:
          bl += 8;
          blChange = true;
          break;
        case KDOWN:
          bl -= 8;
          blChange = true;
          break;
        case KRIGHT:
          manRelay++;
          break;
        case KLEFT:
          manRelay--;
          break;
        case KCENTER:
          mb.Coil (manRelay, ! mb.Coil (manRelay));
          break;
      }
      manRelay %= 5;
    }
  }
  hmi.lcd.setCursor (1, manRelay);

  if (blChange) {

    EEPROM.write (eeBl, bl);
    if (blState) {
      hmi.backlight.write (bl);
    }
  }

  if (blState) {
    if ( (millis() - blTime) >= blTimeout) {

      hmi.backlight.write (0);
      blState = false;
    }
  }

  for (int i = 0; i < 5; i++) {

    bool s = mb.Coil (i);

    if (s != rl[i].state()) {
      if (s) {

        rl[i].set();
        dev |= 1 << i;
      }
      else {

        rl[i].reset();
        dev &= ~ (1 << i);
      }
      hmi.lcd.setCursor (1, i);
      hmi.lcd.write (rl[i].state() ? 'X' : '-');
      devChange = true;
    }
  }

  if (devChange) {

    EEPROM.write (eeDev, dev);
    devChange = false;
  }
}
