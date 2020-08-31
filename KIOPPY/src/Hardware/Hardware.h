#pragma once

extern bool doorOpen;

extern void unlockDoor();
void hardwareInit();
void turnFanOn();
void turnFanOff();
extern void doorIntEn();
extern bool doorIsOpen();