// GsmDuino SMS Server with Ping-Pong feature

// Wait for SMS reception and display them.
// If the SMS text received contains PING in upper case, a PONGn response
// is sent to the sender.

// Created 10 November 2018
// by Pascal JEAN https://github.com/epsilonrt

// This example code is in the public domain.
#include <piduino/configfile.h>
#include <Piduino.h>  // All the magic is here ;-)
#include <modbuspp.h>
#include "gsmduino.h"
#include "sender.h"

using namespace GsmDuino;
using namespace Modbus;

Module  gsm;
HardwareSerial & gsmSerial = Serial1;
Master * mb;

// Config
String gsm_password;
String gsm_serial;
String gsm_db;

std::string modbus_serial;
std::string modbus_config;
int modbus_slave;

int ctor_devices;

bool mySmsReceivedCB (unsigned int index, Module * m);
bool readConfig (const String & filename);

// -----------------------------------------------------------------------------
void setup() {
  Console.begin (115200);
  Console.setTimeout (-1);
  Console.println (F ("Toueris SMS Server"));
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
  mb->rtu().setRts (RtsDown);
  mb->rtu().setSerialMode (Rs485);
  if (!mb->setSlave (modbus_slave)) {
    exit (EXIT_FAILURE);
  }
  if (!mb->open ()) { // open a connection
    exit (EXIT_FAILURE);
  }

  // Gsm
  if (!Sender::dbOpen (gsm_db)) {
    exit (EXIT_FAILURE);
  }

  gsmSerial.begin (115200);
  gsm.smsSetReceivedCB (mySmsReceivedCB);

  PinStatus ps = gsm.begin (gsmSerial);
  if (ps == WaitingPin) {
    String pin;

    Console.print (F ("Pin ? "));
    Console.flush();
    pin = Console.readStringUntil ('\n');
    ps = gsm.begin (gsmSerial, pin);
  }

  if (ps != PinReady) {

    Console.println (F ("Error: Unable to start the GSM module, check its connection and startup !"));
    exit (EXIT_FAILURE);
  }

  if (!gsm.waitRegistration (5000)) {

    Console.println (F ("Error: Network registration failed !"));
    exit (EXIT_FAILURE);
  }
  gsm.smsDeleteAll();

  Console.println (F ("GSM module started successfully !\r\n"));

#ifdef __unix__
  Console.println (F ("Press Ctrl+C to abort ..."));
#endif
}

// -----------------------------------------------------------------------------
void loop () {

  gsm.poll (2000);
}

/*
  Les commandes peuvent être en majuscules ou minuscules.
  Pour effectuer des actions et recevoir des informations, une ligne
  téléphonique s'enregistre à l'aide du message LOGXXXX, XXXX étant
  le code secret (chiffres ou lettres).
  En cas d'échec, aucune réponse n'est envoyée.
  En cas de succès, LOGON est envoyée et la ligne est enregistrée comme
  ligne autorisée.
  Pour se dé-enregistrer, une ligne doit envoyer EXIT (réponse LOGOUT).

  Commande appareils
    La commande de mise en marche est SX (SET), X étant le numéro de la voie,
    réponse ON ou FAIL. La commande d'arrêt est CX (CLEAR), réponse OFF ou FAIL.

  Lecture valeurs
    La commande de lecture
    de l'état d'un appareil est DX (DEVICE),
    pour une voie TOR c'est BX (BINARY),
    pour une tension VX (VOLTAGE) et
    TX pour la température.
    La réponse est ON/OFF/FAIL pour D ou B,
    VX*XX.X pour une tension VX à XX.X Volts,
    TX*±XX.X pour une température et
    HX*±XX.X pour une hygrométrie.

  Trap changement d'état
    Un message BXON/BXOFF est envoyé à toutes les lignes enregistrées en cas
    de changement d'état d'une voie TOR.

  Envoi planifié
    Une ligne enregistrée peut envoyer WDHHMM pour recevoir un SMS
    hebdomadaire (D jour 1 pour lundi, HH heure, MM minute).
    Elle reçoit alors un SMS contenant l'état des appareils, des TOR
    et des tension (DXON, DXOFF, BXON, BXOFF, VX*XX.X),
    chaque élément est séparé par un #.
    Le message WOFF permet d'arrêter le service.
 */


// -----------------------------------------------------------------------------
// SMS received callback
// this function is called by the polling loop when a new SMS arrives,
// the m pointer can be used to access the module.
bool mySmsReceivedCB (unsigned int index, Module * m) {
  Sms sms;

  if (m->smsRead (sms, index)) {
    Sender sender (*m, sms.destination());
    String text = sms.text();

    text.toUpperCase();

    //  Une ligne téléphonique s'enregistre à l'aide du message LOGXXXX,
    //  XXXX étant le code secret (chiffres ou lettres).
    //  En cas d'échec, aucune réponse n'est envoyée.
    //  En cas de succès, LOGON est envoyée et la ligne est enregistrée comme
    //  ligne autorisée.
    if (text.startsWith ("LOG")) {

      if (text.substring (3) == gsm_password) {

        if (sender.sendResponse ("LOGON")) {

          sender.setRegister (true);
          sender.saveToDb();
        }
      }
    }
    else {
      if (sender.isRegister()) {

        if (text.startsWith ("EXIT")) {
          if (sender.sendResponse ("LOGOUT")) {

            sender.setRegister (false);
            sender.saveToDb();
          }
        }
        else {
          int i = text.substring (1).toInt();

          //  Commande appareils
          //    La commande de mise en marche est SX (SET),
          //    X étant le numéro de la voie, réponse ON ou FAIL.
          //    La commande d'arrêt est CX (CLEAR), réponse OFF ou FAIL.
          if (text.startsWith ("S")) {

            if (i <= ctor_devices) {

              if (mb->writeCoil (i, true)) {
                sender.sendResponse ("ON");
              }
              else {
                sender.sendResponse ("FAIL");
              }
            }
            else {
              sender.sendResponse ("FAIL");
            }
          }
          else if (text.startsWith ("C")) {

            if (i <= ctor_devices) {

              if (mb->writeCoil (i, false)) {
                sender.sendResponse ("OFF");
              }
              else {
                sender.sendResponse ("FAIL");
              }
            }
            else {
              sender.sendResponse ("FAIL");
            }

          }
          // La commande de lecture de l'état d'un appareil est DX (DEVICE),
          // La réponse est ON/OFF/FAIL.
          else if (text.startsWith ("D")) {
            if (i <= ctor_devices) {
              bool value;

              if (mb->readCoils (i, &value)) {
                if (value) {
                  sender.sendResponse ("ON");
                }
                else {
                  sender.sendResponse ("OFF");
                }
              }
              else {
                sender.sendResponse ("FAIL");
              }
            }
            else {
              sender.sendResponse ("FAIL");
            }
          }
          else if (text.startsWith ("B")) {

          }
          else if (text.startsWith ("V")) {

          }
          else if (text.startsWith ("T")) {

          }
          else if (text.startsWith ("H")) {

          }
          else if (text.startsWith ("W")) {

          }
        }
      }
    }

    m->smsDelete (index);
    return true;
  }
  return false;
}

// -----------------------------------------------------------------------------
bool readConfig (const String & filename) {

  try {
    Piduino::ConfigFile cfg (filename);
    String str;

    // GSM
    if (!cfg.keyExists ("gsm_password")) {
      return false;
    }
    gsm_password = cfg.value ("gsm_password");

    if (!cfg.keyExists ("gsm_serial")) {
      return false;
    }
    gsm_serial = cfg.value ("gsm_serial");

    if (!cfg.keyExists ("gsm_db")) {
      return false;
    }
    gsm_db = cfg.value ("gsm_db");

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

// -----------------------------------------------------------------------------
