
#include "sender.h"
#include <iostream>

cppdb::session Sender::_db;

// -----------------------------------------------------------------------------
bool Sender::registeredSenders (std::vector<Sender> & senders) {
  return false;
}

// -----------------------------------------------------------------------------
bool Sender::dbOpen (const std::string & connection) {

  try {
    _db.open (connection);
  }
  catch (std::exception const &e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return false;
  }

  return true;
}

// -----------------------------------------------------------------------------
bool Sender::dbIsOpen () {
  return _db.is_open();
}

// -----------------------------------------------------------------------------
bool  Sender::saveToDb() {
  try {

    cppdb::statement stat;

    if (senderExists (_number)) {

      stat = _db <<
             "UPDATE sender "
             "SET registred=?,watch=? "
             "WHERE number=?"
             << _isregister << _watch << _number;

    }
    else {

      stat = _db <<
             "INSERT INTO sender(number,registred,watch) "
             "VALUES(?,?,?)"
             << _number << _isregister << _watch;
    }
    stat.exec();
  }
  catch (std::exception const &e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return false;
  }

  return true;
}

// -----------------------------------------------------------------------------
void Sender::setNumber (const String & number) {

  if (senderExists (number)) {
    try {
      // Load data from db
      cppdb::result res = _db << "SELECT registred,watch FROM sender WHERE number=?" << number << cppdb::row;
      if (!res.empty()) {
        int i;

        res >> i >> _watch;
        _isregister = (i != 0);
      }
    }
    catch (std::exception const &e) {
      std::cerr << "ERROR: " << e.what() << std::endl;
    }
  }
  _number = number;
}

// -----------------------------------------------------------------------------
void Sender::setRegister (bool reg) {
  _isregister = reg;
}

// -----------------------------------------------------------------------------
bool Sender::senderExists (const String & number) {

  try {
    cppdb::result res = _db << "SELECT 1 FROM sender WHERE number=?" << number << cppdb::row;
    if (!res.empty()) {
      return true;
    }
  }
  catch (std::exception const &e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
  }
  return false;
}
