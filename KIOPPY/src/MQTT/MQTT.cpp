
#include "MQTT.h"
#include "ArduinoLog.h"
#include "DHT/myDht.h"
#include "ArduinoJson.h"

// char payload[2048] = {0};

//String payload;
void mqttSendMessage(mqttStruct_t msg)
{
    xQueueSendToBack(mqttQ, &msg, portMAX_DELAY);
}

// void mqttSendMessage(mqttMsgType_t msgType)
// {
//     switch (msgType)
//     {
//     case DOOR:
//     {
//         mqttStruct_t door;
//         door.msgType = msgType;
//         door.open = 0;
//         xQueueSendToBack(mqttQ, &door, portMAX_DELAY);
//         break;
//     }
//     case TAKE_MED:
//     {
//         mqttStruct_t takeMedicine;

//         takeMedicine.msgType = msgType;
//         strcpy(takeMedicine.barcode, "12345566666");
//         takeMedicine.qty = 200;

//         xQueueSendToBack(mqttQ, &takeMedicine, portMAX_DELAY);
//         break;
//     }
//     case PUT_MED:
//     {
//         mqttStruct_t putMedicine;

//         // .event = MQTT_SEND_DATA_EVT,
//         putMedicine.msgType = msgType;               //msgType
//         strcpy(putMedicine.barcode, "123456798AFV"); //barcode
//         strcpy(putMedicine.expDate , "31/01/2020");  //expDate

//         xQueueSendToBack(mqttQ, &putMedicine, portMAX_DELAY);
//         break;
//     }

//     case SENSORS:
//     {
//         Log.verbose("Send Senors to Queue" CR);
//         mqttStruct_t params;
//         // .event = MQTT_SEND_DATA_EVT,
//         params.msgType = msgType;                      //msgType
//         params.humidity = getDhtData().humidity;       //Hum
//         params.temperature = getDhtData().temperature; //Temp

//         xQueueSendToBack(mqttQ, &params, portMAX_DELAY);
//         break;
//     }

//     case TIME_SYNC:
//     {
//         break;
//     }

//     default:
//         break;
//     }
// }

uint8_t jsonDeserializer(const char *jsonStr, const char *barcode, const char *name, const char *type)
{
    const size_t capacity = JSON_OBJECT_SIZE(4) + 150;
    DynamicJsonDocument doc(capacity);
    deserializeJson(doc, jsonStr);

    uint8_t msgType = doc["msg_type"];
    if (msgType == 9)
    {
        barcode = doc["barcode"];
        name = doc["name"];
        type = doc["type"];
        return 1;
    }
    else
    {
        return 0;
    }
}
char booltochar[2][6] = {"false", "true"};

static String structToJson(mqttStruct_t *msg)
{

    String output;

    if (msg->msgType == DOOR)
    {
        const int capacity = JSON_OBJECT_SIZE(2);
        StaticJsonDocument<capacity> doc;
        doc["msg_type"] = msg->msgType;
        doc["open"] = msg->open;
        serializeJson(doc, output);
    }
    else if (msg->msgType == PUT_MED)
    {
        const int capacity = JSON_OBJECT_SIZE(3);
        StaticJsonDocument<capacity> doc;
        doc["msg_type"] = msg->msgType;
        doc["barcode"] = msg->barcode;
        doc["exp_date"] = msg->expDate;

        serializeJson(doc, output);
    }
    else if (msg->msgType == TAKE_MED)
    {
        const int capacity = JSON_OBJECT_SIZE(3);
        StaticJsonDocument<capacity> doc;
        doc["msg_type"] = msg->msgType;
        doc["barcode"] = msg->barcode;
        doc["qty"] = msg->qty;

        serializeJson(doc, output);
    }
    else if (msg->msgType == SENSORS)
    {

        //     Log.verbose("Serialize Sensors" CR);
        const int capacity = JSON_OBJECT_SIZE(3);
        StaticJsonDocument<capacity> doc;

        doc["msg_type"] = msg->msgType;
        doc["humidity"] = msg->humidity;
        doc["temperature"] = msg->temperature;
        serializeJson(doc, output);
        // Serial.println(output);
    }
    else if (msg->msgType == CONNECTED)
    {
        // target += sprintf(target, "{\"msg_type\": %d,}", msg->msgType);
        const int capacity = JSON_OBJECT_SIZE(1);
        StaticJsonDocument<capacity> doc;
        doc["msg_type"] = msg->msgType;
        serializeJson(doc, output);
    }
    else if (msg->msgType == REMOVE_MED)
    {
        const int capacity = JSON_OBJECT_SIZE(2);
        StaticJsonDocument<capacity> doc;
        doc["msg_type"] = msg->msgType;
        doc["barcode"] = msg->barcode;
        serializeJson(doc, output);
    }
    return output;
    // printf("%s", target);
    // fflush(stdout);
}

// static void structToJson(mqttStruct_t *msg, char *target)
// {

//     String output;

//     if (msg->msgType == DOOR)
//     {
//         /* const int capacity = JSON_OBJECT_SIZE(2);
//         StaticJsonDocument<capacity> doc;
//         doc["msg_type"] = msg->msgType;
//         doc["open"] = msg->open;
//         serializeJson(doc, output);*/

//         target += sprintf(target, "{\"msg_type\": %d,", msg->msgType);
//         target += sprintf(target, "\"open\": %s", booltochar[msg->open]);
//         target += sprintf(target, "}");
//     }
//     else if (msg->msgType == PUT_MED)
//     {
//         /* const int capacity = JSON_OBJECT_SIZE(3);
//         StaticJsonDocument<capacity> doc;
//         doc["msg_type"] = msg->msgType;
//         doc["barcode"] = msg->barcode;
//         doc["exp_date"] = msg->expDate;

//         serializeJson(doc, output);*/
//         target += sprintf(target, "{\"msg_type\": %d,", msg->msgType);
//         target += sprintf(target, "\"barcode\": \"%s\",", msg->barcode);
//         target += sprintf(target, "\"exp_date\": \"%s\"}", msg->expDate);
//     }
//     else if (msg->msgType == TAKE_MED)
//     {
//         target += sprintf(target, "{\"msg_type\": %d,", msg->msgType);
//         target += sprintf(target, "\"barcode\": \"%s\",", msg->barcode);
//         target += sprintf(target, "\"qty\": %d}", msg->qty);

//         /* const int capacity = JSON_OBJECT_SIZE(3);
//         StaticJsonDocument<capacity> doc;
//         doc["msg_type"] = msg->msgType;
//         doc["barcode"] = msg->barcode;
//         doc["qty"] = msg->qty;

//         serializeJson(doc, output);*/
//     }
//     else if (msg->msgType == SENSORS)
//     {
//         Log.verbose("Serialize Data" CR);

//         target += sprintf(target, "{\"msg_type\": %d,", msg->msgType);
//         target += sprintf(target, "\"humidity\": %0.2f,", msg->humidity);
//         target += sprintf(target, "\"temperature\": %0.2f}", msg->temperature);
//         /*
//         //     Log.verbose("Serialize Sensors" CR);
//         const int capacity = JSON_OBJECT_SIZE(3);
//         StaticJsonDocument<capacity> doc;

//         doc["msg_type"] = msg->msgType;
//         doc["humidity"] = msg->humidity;
//         doc["temperature"] = msg->temperature;
//         serializeJson(doc, output);
//         Serial.println(output);*/
//     }
//     else if (msg->msgType == CONNECTED)
//     {
//         target += sprintf(target, "{\"msg_type\": %d,}", msg->msgType);
//     }
//     // return output;
//     printf("%s", target);
//     fflush(stdout);
// }

void mqttTask(void *pvParameters)
{

    mqttStruct_t mqttReceive;
    while (1)
    {
        if (xQueueReceive(mqttQ, (void *)&mqttReceive,
                          (portTickType)portMAX_DELAY))
        {
            // String payload;
            // structToJson(&mqttReceive, payload);

            String p = structToJson(&mqttReceive);

            //  Log.verbose("send Serialized Data %S" CR, payload.c_str());
            client.publish(topicName + "/TX", p);
            vTaskDelay(pdMS_TO_TICKS(100));
            client.publish("SmartCabinet/12345/TX", p);

            // esp_mqtt_client_publish(client, "SmartCabinet/12345/TX",
            //                         payload, 0, 1, 0);
            // break;
        }
    }
}

void createMqttTask()
{
    xTaskCreate(mqttTask, "mqttTask", 2048, NULL, 5, NULL);
}

void onConnectionEstablished()
{
    Log.verbose("MQTT COnnected" CR);
    // Subscribe to "mytopic/test" and display received message to Serial
    client.subscribe(topicName + "/RX", [](const String &payload) {
        // Log.verbose("Payload: %s" CR, payload);

        const size_t capacity = JSON_OBJECT_SIZE(4) + 150;
        DynamicJsonDocument doc(capacity);
        deserializeJson(doc, payload);

        uint8_t msgType = doc["msg_type"];
        if (msgType == 9)
        {
            const char *barcode = doc["barcode"];
            const char *name = doc["name"];
            const char *type = doc["type"];
        }
    });

    mqttStruct_t con;
    con.msgType = CONNECTED; //msgType
    xQueueSendToBack(mqttQ, &con, portMAX_DELAY);

    // client.publish(topicName, "Success", true);
}
