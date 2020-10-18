#pragma once
#include "Arduino.h"
#include "EspMQTTClient.h"
#include "esp_log.h"

#define SETUP_ADDRESS 1
#define LOCK_CODE_ADDRESS 2 //1+1
#define BLOWER_ADDRESS 7 //2+4
#define SSID_ADDRESS 10
#define PASSWORD_ADDRESS 50


extern bool blwrEnable; 
extern bool cancelScan;
typedef enum
{
    MAX_TEMP_REACHED_EVT = 0,
    MIN_TEMP_REACHED_EVT,
    DHT_TIMER_EVT,
    DOOR_EVT,
    NTP_TIMER_EVT,
    WIFI_RECOONECT_EVT,
    MOBILE_DISCONNECTED_EVT,
    MOBILE_CONNECTED_EVT
} customEvents_t;

typedef struct
{
    customEvents_t event;
} eventStruct_t;


enum ScannedMed {TAKE_MED_SCANNED, PUT_MED_SCANNED, NO_MED_SCANNED};
extern enum ScannedMed scannedMed;
extern QueueHandle_t monitoringQ;

extern bool fanOn;

extern String wifiSSID;
extern String wifiPassword;

extern EspMQTTClient client;


extern void monitoringQueueAdd(customEvents_t monitor);
void monitoringQueueAddFromISR(customEvents_t event);
void lcdSendCommand(String str);
// extern void setLockCode(String code);
// extern String lockCode;

extern bool boolSmartConfigStop;