#include "bisrelay.h"

void BisRelay::begin() {

  pinMode (_setPin, OUTPUT);
  digitalWrite (_setPin, LOW);
  pinMode (_resetPin, OUTPUT);
  digitalWrite (_resetPin, LOW);
  reset();
}

void BisRelay::set (bool s) {
  int pin = s ? _setPin : _resetPin;
  digitalWrite (pin, HIGH);
  ::delay (_delay);
  digitalWrite (pin, LOW);
  _state = s;
}
