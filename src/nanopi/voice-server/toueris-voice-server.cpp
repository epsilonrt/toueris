// Toueris Voice Server
//
// This example shows how to manage a voice server.
// The program waits for a call, counts the number of rings and picks up
// the phone after 3 rings.
// After a welcome message, the program waits for the other party to press
// keys and diffuse a different message for each one.
// If the other party presses 2 # keys successively, the server says a
// goodbye message and hangs up the phone.
//
// This example code is in the public domain.
#include <atomic>
#include <sstream>
#include <cctype>
#include <piduino/configfile.h>
#include <Piduino.h>  // All the magic is here ;-)
#include <modbuspp.h>
#include <piphons.h>

using namespace std;
using namespace Piphons;
using namespace Modbus;

// Config
string voice_password;
string voice_lang ("en-US");
int voice_ringbefore_offhook = 3;
bool voice_flash_offhook = false;

string modbus_serial;
string modbus_config;
int modbus_slave;
bool modbus_rs485 = false;

int ctor_devices;

// Hardware
const int ringPin = 6;  // Header Pin 22: GPIO6 for RPi, GPIOA1 for NanoPi
const int offhookPin = 5; // Header Pin 18: GPIO7 for RPi, GPIOG9 for NanoPi
const int tonePin = 11; // Header Pin 26: GPIO24 for RPi, GPIOA17 for NanoPi
const std::array<int, 5> dtmfPins = {0, 1, 2, 3, 4}; // D0, D1, D2, D3, DV

// Globals
Master * mb;
Tts tts; // object for speech synthesis, to talk to each other.
Daa daa (ringPin, offhookPin, tonePin);
Dtmf dtmf (dtmfPins);

atomic<bool> run (true); // Atomic Boolean variable, needed because the handlers run in different threads...
atomic<bool> logon (false);
ostringstream out;

// Functions
void keyIsr (Dtmf * dtmf);
void offhookIsr (Daa * daa);
void ringIsr (Daa * daa);
bool readConfig (const String & filename);

// -----------------------------------------------------------------------------
void setup() {
  Console.begin (115200);
  Console.setTimeout (-1);
  Console.println (F ("Toueris Voice Server"));
  Console.println (F ("Waiting to initialize the module, may take a little while..."));

  String cfg_filename = "/etc/toueris.conf";
  if (argc > 1) {
    cfg_filename = String (argv[1]);
  }
  if (!readConfig (cfg_filename)) {
    exit (EXIT_FAILURE);
  }

  // Modbus
  mb = new Master (Rtu, modbus_serial, modbus_config); // new master on RTU
  // if you have to handle the DE signal of the line driver with RTS,
  // you should uncomment the lines below...
  if (modbus_rs485) {

    mb->rtu().setRts (RtsDown);
    mb->rtu().setSerialMode (Rs485);
  }
  if (!mb->setSlave (modbus_slave)) {
    exit (EXIT_FAILURE);
  }
  if (!mb->open ()) { // open a connection
    exit (EXIT_FAILURE);
  }

  // Phone Voice
  daa.setRingingHandler (ringIsr);
  daa.setOffhookHandler (offhookIsr);
  daa.setHookFlash (voice_flash_offhook);
  daa.setRingingBeforeOffhook (voice_ringbefore_offhook);

  if (!daa.open()) {
    Console.println (F ("Unable to open DAA !"));
    exit (EXIT_FAILURE);
  }

  dtmf.setKeyHandler (keyIsr);
  if (!dtmf.open()) {
    Console.println (F ("Unable to open DTMF !"));
    exit (EXIT_FAILURE);
  }

  if (!tts.open (voice_lang)) {
    Console.print (F ("Unable to open TTS : "));
    Console.println (tts.errorString());
    exit (EXIT_FAILURE);
  }

  out << "Sélectionnez la commande 1 à " << ctor_devices << " ou raccrochez !";
  Console.println (F ("Voice server started successfully !\r\n"));

#ifdef __unix__
  Console.println (F ("Press Ctrl+C to abort ..."));
#endif
}

// -----------------------------------------------------------------------------
void loop () {

  while (run.load()) {

    delay (100);
  }

  tts.close();
  dtmf.close();
  daa.close();
  Console.println (F ("I hung up the phone."));
  Console.println (F ("Have a nice day !"));

  exit (EXIT_SUCCESS);
}

// -----------------------------------------------------------------------------
void keyIsr (Dtmf * dtmf) {
  static char prev;
  char c;
  static string text;
  static int device = -1;

  c = dtmf->read(); // get the key
  Console.write (c); // print the key to stdout
  Console.flush();

  if (!logon.load()) {
    text += c;
    if (text.size() == voice_password.size()) {

      if (text == voice_password) {
        logon.store (true);
        tts.say (out.str());
      }
      else {
        text.clear();
        daa.hangup();
      }
    }
  }
  else {
    // logon
    if (c != '#') {

      if ( (c == '*') && (device >= 0)) {
        bool value;

        if (mb->readCoils (device, &value)) {

          if (mb->writeCoil (device, !value)) {

            c = device + '0';
          }
        }
      }

      if (isdigit (c)) {
        device = c - '0';

        if ( (device >= 1) && (device <= ctor_devices)) {
          bool value;

          if (mb->readCoils (device, &value)) {

            text = "La commande ";
            text += c;

            if (value) {

              text += " est en marche.";
            }
            else {

              text += " est en arrêt.";
            }
          }
          else {

            text = out.str();
          }
        }
        else {

          text = out.str();
        }
      }
      else {

        text = out.str();
      }
      tts.say (text);
    }
  }

  if ( (c == '#') && (prev == '#')) {
    // 2 # sent consecutively, this is the signal to stop

    tts.say ("Au revoir !"); // goodbye message
    run = false; // drop the flag atomically to tell the main thread to stop
  }

  prev = c; // memorizes the previous character
}

// -----------------------------------------------------------------------------
void offhookIsr (Daa * daa) { // this function is executed after hookoff

  Console.println (F ("Off-hook !"));
  delay (2000); // delay for the establishment of the telephone line.
  tts.say ("Bonjour!", 100, 80);
  tts.say ("Composez votre code secret !", 80, 80);
}

// -----------------------------------------------------------------------------
void ringIsr (Daa * daa) { // this function is executed at each ring

  // displays the number of rings since the last call
  Console.print (daa->ringingSinceHangup());
  Console.flush();
}


// -----------------------------------------------------------------------------
bool readConfig (const String & filename) {

  try {
    Piduino::ConfigFile cfg (filename);
    String str;

    // Voice
    if (!cfg.keyExists ("voice_password")) {
      return false;
    }
    voice_password = cfg.value ("voice_password");

    if (cfg.keyExists ("voice_lang")) { // Optional
      voice_lang = cfg.value ("voice_lang");
    }

    if (cfg.keyExists ("voice_ringbefore_offhook")) { // Optional
      str = cfg.value ("voice_ringbefore_offhook");
      voice_ringbefore_offhook = str.toInt();
    }

    if (cfg.keyExists ("voice_flash_offhook")) { // Optional
      str = cfg.value ("voice_flash_offhook");
      voice_flash_offhook = (str.toInt() != 0);
    }

    // MODBUS
    if (!cfg.keyExists ("modbus_serial")) {
      return false;
    }
    modbus_serial = cfg.value ("modbus_serial");

    if (!cfg.keyExists ("modbus_slave")) {
      return false;
    }
    str = cfg.value ("modbus_slave");
    modbus_slave = str.toInt();

    if (cfg.keyExists ("modbus_rs485")) { // Optional
      str = cfg.value ("modbus_rs485");
      modbus_rs485 = (str.toInt() != 0);
    }

    if (!cfg.keyExists ("modbus_config")) {
      modbus_config = String ("19200E1");
    }
    else {
      modbus_config = cfg.value ("modbus_config");
    }

    if (!cfg.keyExists ("ctor_devices")) {
      return false;
    }
    str = cfg.value ("ctor_devices");
    ctor_devices = str.toInt();
  }
  catch (...) {

    Console.print ("Unable to read ");
    Console.println (filename);
    return false;
  }
  return true;
}


/* ========================================================================== */
