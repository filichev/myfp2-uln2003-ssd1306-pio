// Minimal EEPROM helpers compatible with myFocuserPro2 usage
#ifndef MY_EEPROM_H
#define MY_EEPROM_H

#include <Arduino.h>
#include <EEPROM.h>

// Write any POD struct/primitive to EEPROM starting at 'index'
// Returns number of bytes written
template <class T>
int EEPROM_writeAnything(int index, const T &value) {
  const byte *p = (const byte *)(const void *)&value;
  for (unsigned int i = 0; i < sizeof(T); i++) {
    EEPROM.update(index++, *p++);
  }
  return sizeof(T);
}

// Read any POD struct/primitive from EEPROM starting at 'index'
// Returns number of bytes read
template <class T>
int EEPROM_readAnything(int index, T &value) {
  byte *p = (byte *)(void *)&value;
  for (unsigned int i = 0; i < sizeof(T); i++) {
    *p++ = EEPROM.read(index++);
  }
  return sizeof(T);
}

#endif  // MY_EEPROM_H
