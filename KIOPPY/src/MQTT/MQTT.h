#pragma once
#include "Arduino.h"
#include "Global.h"

typedef struct
{
    // mqttEvt_t event; /**< @brief MQTT events*/
    uint8_t msgType; /**< @brief Message type (door, medicine...)*/
    bool open;
    String barcode;
    String expDate;
    String name;
    String form;
    // char barcode[49];
	// char expDate[11];
    float qty;
    float humidity;
    float temperature;
} mqttStruct_t;

typedef enum
{
    DOOR = 1, 
    TAKE_MED, 
    PUT_MED,
    SENSORS,
    TIME_SYNC,
    CONNECTED,
    REMOVE_MED,
    SCAN_MED,
    MED_DETAILS,
    PHONE_CONNECTED,
    NEW_MED,
} mqttMsgType_t;

extern QueueHandle_t mqttQ;
extern String topicName;
extern void onConnectionEstablished();
extern void createMqttTask();
extern void mqttSendMessage(mqttMsgType_t msgType);
