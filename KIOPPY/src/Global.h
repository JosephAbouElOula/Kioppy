#pragma once
#include "Arduino.h"
#include "EspMQTTClient.h"
#include "esp_log.h"
typedef enum
{
    MAX_TEMP_REACHED_EVT = 0,
    MIN_TEMP_REACHED_EVT,
    DHT_TIMER_EVT,
    DOOR_EVT,
    OPEN_DOOR_EVT,
    CHANGE_WIFI,
} customEvents_t;

typedef struct
{
    customEvents_t event;
} eventStruct_t;


enum ScannedMed {TAKE_MED_SCANNED, PUT_MED_SCANNED, NO_MED_SCANNED};
extern enum ScannedMed scannedMed;
extern QueueHandle_t monitoringQ;

extern bool fanOn;

extern EspMQTTClient client;


extern void monitoringQueueAdd(customEvents_t monitor);
void monitoringQueueAddFromISR(customEvents_t event);
void lcdSendCommand(String str);
// extern void setLockCode(String code);
// extern String lockCode;

extern bool boolSmartConfigStop;