#pragma once
#include <EEPROM.h>
void writeStringToEEPROM(char add, String data);
String readStringFromEEPROM(char add);