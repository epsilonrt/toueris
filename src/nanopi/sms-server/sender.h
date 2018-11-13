
#ifndef SENDER_H
#define SENDER_H
#include <vector>
#include <cppdb/frontend.h>
#include <Arduino.h>
#include "gsmduino.h"

class Sender {
  public:
    Sender (GsmDuino::Module & module) : _m (module), _isregister (false) {}
    Sender (GsmDuino::Module & module, const String & number) : Sender (module) {
      setNumber (number);
    }
    ~Sender() {}

    void setNumber (const String & number);
    void setWatch (const String & watch);
    void setRegister (bool reg);
    bool saveToDb();

    const String & number() const {
      return _number;
    }
    const String & watch() const {
      return _watch;
    }
    bool isRegister() const {
      return _isregister;
    }
    bool sendResponse (const String & response) {
      GsmDuino::Sms sms (response, _number);

      return _m.smsSend (sms);
    }

    static bool registeredSenders (std::vector<Sender> & senders);
    static bool senderExists (const String & number);
    static bool dbOpen (const std::string & connection);
    static bool dbIsOpen ();

  private:
    GsmDuino::Module & _m;
    bool _isregister;
    String _number;
    String _watch;
    static cppdb::session _db;
};

#endif
