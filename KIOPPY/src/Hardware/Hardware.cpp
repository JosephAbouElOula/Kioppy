/*
* Includes
*/
#include "Hardware.h"
#include "Arduino.h"
#include "DHT/myDht.h"
#include "Global.h"
#include "Nextion/Nextion.h"
#include "Barcode/Barcode.h"
#include "EEPROM.h"

/*
* defines
*/
#define FAN_PIN 33
#define LOCK_PIN 32
#define DOOR_DET_PIN 26


const int doorTimeOut = 3000;
/*
* Variables
*/
TimerHandle_t doorTimer;

bool doorOpen = false;

void turnFanOff()
{
    Serial.println("Fan OFF");
    digitalWrite(FAN_PIN, 0);
    fanOn = false;
}

void turnFanOn()
{
    Serial.println("Fan On");
    digitalWrite(FAN_PIN, 1);
    fanOn = true;
}

void unlockDoor()
{
    digitalWrite(LOCK_PIN, HIGH);
    xTimerStart(doorTimer, 0);
}

void IRAM_ATTR doorDetInt()
{
    detachInterrupt(DOOR_DET_PIN);
    monitoringQueueAddFromISR(DOOR_EVT);
 

    // monitoringQueueAdd(DOOR_EVT);
}
void doorIntEn()
{
    attachInterrupt(DOOR_DET_PIN, doorDetInt, CHANGE);
}
void IRAM_ATTR doorTimerCallBack(TimerHandle_t doorTimer)
{
    digitalWrite(LOCK_PIN, LOW);
}

int getDoorState()
{
    return (digitalRead(DOOR_DET_PIN));
}

bool doorIsOpen()
{
    if (getDoorState() == HIGH)
    {
        return true;
    }
    else
    {
        return false;
    }
}
void hardwareInit()
{
    pinMode(LOCK_PIN, OUTPUT);
    digitalWrite(LOCK_PIN, LOW);

    pinMode(FAN_PIN, OUTPUT);
    turnFanOff();
    pinMode(DOOR_DET_PIN, INPUT);
    // attachInterrupt(DOOR_DET_PIN, doorDetInt, CHANGE);
    doorIntEn();
    doorTimer = xTimerCreate("doorTimer", doorTimeOut / portTICK_PERIOD_MS, pdFALSE, (void *)0, doorTimerCallBack);
    nextionUartInit();
    barcodeUartInit();
    
    if(EEPROM.read(BLOWER_ADDRESS)==0){
        blwrEnable=0;
    } else {
        blwrEnable=1;
    }


    initDHT(10);
    
}
