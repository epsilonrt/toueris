#ifndef BIS_RELAY_H
#define BIS_RELAY_H 1
#include <Arduino.h>

class BisRelay {
  public:
    BisRelay (int setPin, int resetPin, unsigned settingDelay = 10) :
      _setPin (setPin), _resetPin (resetPin), _state (false), _delay (settingDelay) {}

    void begin();
    void set (bool s = true);
    inline void reset() {
      set (false);
    }
    inline bool state() const {
      return _state;
    }
    inline unsigned delay() const {
      return _delay;
    }
    inline void setDelay (unsigned d) {
      _delay = d;
    }

  private:
    int _setPin;
    int _resetPin;
    bool _state;
    unsigned _delay;
};

#endif
