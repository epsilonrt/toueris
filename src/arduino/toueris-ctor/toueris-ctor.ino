/*
 * TOUERIS MODBUS CTOR SLAVE
 */
#include <EEPROM.h>
#include <Modbus.h>
#include <ModbusSerial.h> // https://github.com/epsilonrt/modbus-arduino
#include <Toueris2Hmi.h>  // https://github.com/epsilonrt/WireHmi
#include <LCD_ST7032.h> // https://github.com/epsilonrt/LCD_ST7032
#include <PinChangeInterrupt.h> // https://github.com/NicoHood/PinChangeInterrupt
#include "bisrelay.h"

// Modbus
const byte MODBUS_SLAVEID = 8;
const unsigned long MODBUS_BAUDRATE = 19200;
const byte MODBUS_CONFIG = MB_PARITY_EVEN;
const char MODBUS_STR_CONFIG[] = "8E1";

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
Toueris2Hmi hmi (hirqPin);
// ST7032 LCD
LCD_ST7032 lcd;

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
  int loops = 0;

  // starts the LCD, need to repeat the call in the case of a USB power boot...
  for (int i = 0; i < 2; i++) {
    lcd.begin();  // this function calls Wire.begin(), no need to double the call!
  }
  lcd.setcontrast (24); // contrast value range is 0-63, try 25@5V or 50@3.3V as a starting value
  lcd.print ("Waiting HMI...");

  dev = EEPROM.read (eeDev);
  if (dev & 0xE0) {
    dev = 0;
    EEPROM.write (eeDev, dev);
  }
  lcd.setCursor (1, 0);
  lcd.print (dev, HEX);

  while (!hmi.begin()) { // start the HMI by checking that it has worked well...
    lcd.setCursor (1, 0);
    lcd.print (++loops);
    delay (500);
  }

  bl = EEPROM.read (eeBl) & 0xF8; // rounded to a multiple of 8
  hmi.backlight.write (bl);
  blTime = millis();

  // Config Modbus Serial (port, speed, byte format)
  mb.config (&Serial1, MODBUS_BAUDRATE, MODBUS_CONFIG, dePin);
  // Set the Slave ID (1-247)
  mb.setSlaveId (MODBUS_SLAVEID);

  lcd.clear();
  //          0123456789012345
  lcd.print ("12345| CTOR @");
  lcd.print (MODBUS_SLAVEID);


  lcd.setCursor (1, 0);
  for (int i = 0; i < 5; i++) {

    mb.addCoil (i);

    rl[i].begin();
    if (dev & (1 << i)) {
      mb.Coil (i, true);
    }

    lcd.write (rl[i].state() ? 'X' : '-');
  }

  //          0123456789
  lcd.print ("| ");
  lcd.print (MODBUS_BAUDRATE);
  lcd.print (MODBUS_STR_CONFIG);

  attachPCINT (digitalPinToPCINT (icomPin), icomHandler, CHANGE);
  hmi.led.set (LED_GREEN1);
  lcd.cursor();
  lcd.blink();
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
  lcd.setCursor (1, manRelay);

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
      lcd.setCursor (1, i);
      lcd.write (rl[i].state() ? 'X' : '-');
      devChange = true;
    }
  }

  if (devChange) {

    EEPROM.write (eeDev, dev);
    devChange = false;
  }
}
