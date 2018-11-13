// GsmDuino SMS Server with Ping-Pong feature

// Wait for SMS reception and display them.
// If the SMS text received contains PING in upper case, a PONGn response
// is sent to the sender.

// Created 10 November 2018
// by Pascal JEAN https://github.com/epsilonrt

// This example code is in the public domain.
#include <vector>
#include <piduino/configfile.h>
#include <Piduino.h>  // All the magic is here ;-)
#include "gsmduino.h"

using namespace GsmDuino;

Module  gsm;
HardwareSerial & gsmSerial = Serial1;
ConfigFile * cfg;
int devices = 5;

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
    La réponse est ON/OFF pour D ou B,
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
class Sender {
  public:
    Sender (GsmDuino::Module & module) : _m (module), _id (0), _isregister (false) {}
    Sender (GsmDuino::Module & module, const String & number) : Sender (module) {
      setNumber (number);
    }
    Sender (GsmDuino::Module & module, long id) : Sender (module) {
      setId (id);
    }

    void setNumber (const String & number) {}
    void setRegister (bool reg) {}
    void setId (long id) {}

    long id() const {
      return _id;
    }
    const String & number() const {
      return _number;
    }
    bool isRegister() const {
      return _isregister;
    }
    bool sendResponse (const String & response) {
      GsmDuino::Sms sms (response, _number);

      return _m.smsSend (sms);
    }

    static vector<long> registerSenders();
    static String password() {
      return _passwd;
    }
  private:
    GsmDuino::Module & _m;
    long _id;
    bool _isregister;
    String _number;
    static String _passwd;
};

// SMS received callback
// this function is called by the polling loop when a new SMS arrives,
// the m pointer can be used to access the module.
bool mySmsReceivedCB (unsigned int index, Module * m) {
  Sms sms;

  if (m->smsRead (sms, index)) {
    Sender sender (m, sms.destination());
    const String & text = sms.text();

    text.toUpperCase();

    //  Une ligne téléphonique s'enregistre à l'aide du message LOGXXXX,
    //  XXXX étant le code secret (chiffres ou lettres).
    //  En cas d'échec, aucune réponse n'est envoyée.
    //  En cas de succès, LOGON est envoyée et la ligne est enregistrée comme
    //  ligne autorisée.
    if (text.startsWith ("LOG")) {

      if (cfg->keyExists ("password")) {
        String pw = cfg->value("password");
        
        if (text.substring (3) == pw) {
          
          if (sender.sendResponse ("LOGON")) {

            sender.setRegister (true);
          }
        }
      }

    }
    else {
      if (sender.isRegister()) {

        if (text.startsWith ("EXIT")) {
          if (sender.sendResponse ("LOGOUT")) {

            sender.setRegister (false);
          }
        }
        //  Commande appareils
        //    La commande de mise en marche est SX (SET),
        //    X étant le numéro de la voie, réponse ON ou FAIL.
        //    La commande d'arrêt est CX (CLEAR), réponse OFF ou FAIL.
        else if (text.startsWith ("S")) {
          
          int chan = text.subString(1).toInt();

        }
        else if (text.startsWith ("C")) {

        }
        else if (text.startsWith ("D")) {

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

    m->smsDelete (index);
    return true;
  }
  return false;
}

void setup() {

  cfg = new ConfigFile ("toueris.config");

  Console.begin (115200);
  Console.setTimeout (-1);
  Console.println (F ("Toueris SMS Server"));
  Console.println (F ("Waiting to initialize the module, may take a little while..."));

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

void loop () {

  gsm.poll (2000);
}
