
#include "MQTT.h"
#include "ArduinoLog.h"
#include "DHT/myDht.h"
#include "ArduinoJson.h"
#include "Medicine/Medicine.h"
// char payload[2048] = {0};
uint8_t loopIndex = 0;
//String payload;
void mqttSendMessage(mqttStruct_t msg)
{
    if (client.isConnected())
    {
        xQueueSendToBack(mqttQ, &msg, pdMS_TO_TICKS(500));
    }
}

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

static void structToJson(mqttStruct_t *msg, char *target)
{

    String output;

    if (msg->msgType == DOOR)
    {
        /* const int capacity = JSON_OBJECT_SIZE(2);
        StaticJsonDocument<capacity> doc;
        doc["msg_type"] = msg->msgType;
        doc["open"] = msg->open;
        serializeJson(doc, output);*/

        target += sprintf(target, "{\"msg_type\": %d,", msg->msgType);
        target += sprintf(target, "\"open\": %s", booltochar[msg->open]);
        target += sprintf(target, "}");
    }
    else if (msg->msgType == PUT_MED)
    {
        /* const int capacity = JSON_OBJECT_SIZE(3);
        StaticJsonDocument<capacity> doc;
        doc["msg_type"] = msg->msgType;
        doc["barcode"] = msg->barcode;
        doc["exp_date"] = msg->expDate;

        serializeJson(doc, output);*/
        target += sprintf(target, "{\"msg_type\": %d,", msg->msgType);
        target += sprintf(target, "\"barcode\": \"%s\",", msg->barcode.c_str());
        target += sprintf(target, "\"exp_date\": \"%s\"}", msg->expDate.c_str());
    }
    else if (msg->msgType == TAKE_MED)
    {
        target += sprintf(target, "{\"msg_type\": %d,", msg->msgType);
        target += sprintf(target, "\"barcode\": \"%s\",", msg->barcode.c_str());
        target += sprintf(target, "\"qty\": %.2f}", msg->qty);

        /* const int capacity = JSON_OBJECT_SIZE(3);
        StaticJsonDocument<capacity> doc;
        doc["msg_type"] = msg->msgType;
        doc["barcode"] = msg->barcode;
        doc["qty"] = msg->qty;

        serializeJson(doc, output);*/
    }
    else if (msg->msgType == NEW_MED)
    {
        target += sprintf(target, "{\"msg_type\": %d,", msg->msgType);
        target += sprintf(target, "\"barcode\": \"%s\",", msg->barcode.c_str());
        target += sprintf(target, "\"name\": \"%s\",", msg->name.c_str());
        target += sprintf(target, "\"type\": \"%s\",", msg->form.c_str());
        target += sprintf(target, "\"qty\": %d,", msg->initQty / 100);
        target += sprintf(target, "\"exp_date\": \"%s\"}", msg->expDate.c_str());
    }
    else if (msg->msgType == SENSORS)
    {
        Log.verbose("Serialize Data" CR);

        target += sprintf(target, "{\"msg_type\": %d,", msg->msgType);
        target += sprintf(target, "\"humidity\": %0.2f,", msg->humidity);
        target += sprintf(target, "\"temperature\": %0.2f}", msg->temperature);
        /*
        //     Log.verbose("Serialize Sensors" CR);
        const int capacity = JSON_OBJECT_SIZE(3);
        StaticJsonDocument<capacity> doc;

        doc["msg_type"] = msg->msgType;
        doc["humidity"] = msg->humidity;
        doc["temperature"] = msg->temperature;
        serializeJson(doc, output);
        Serial.println(output);*/
    }
    else if (msg->msgType == CONNECTED)
    {
        target += sprintf(target, "{\"msg_type\": %d}", msg->msgType);
    }
    else if (msg->msgType == REMOVE_MED)
    {
        target += sprintf(target, "{\"msg_type\": %d,", msg->msgType);
        target += sprintf(target, "\"barcode\": \"%s\"}", msg->barcode.c_str());
    }
    else if (msg->msgType == SCAN_MED)
    {
        target += sprintf(target, "{\"msg_type\": %d,", msg->msgType);
        target += sprintf(target, "\"barcode\": \"%s\"}", msg->barcode.c_str());
    }
    else if (msg->msgType == INVENTORY)
    {
        target += sprintf(target, "{\"msg_type\": %d, \"Medicines\":[", msg->msgType);
        if (allMedicines->getCounter() > 0)
        {
            target += sprintf(target, "[\"%s\",\"%s\",%.2f,%d,%d]", allMedicines->allMedList[0].getBarcode(), allMedicines->allMedList[0].getDescription(),
                                  allMedicines->allMedList[0].getQty() / 100.0, allMedicines->allMedList[0].getInitialQty() / 100, allMedicines->allMedList[0].getType());
            for (int i = 1; i < allMedicines->getCounter(); ++i)
            {
                target += sprintf(target, ",[\"%s\",\"%s\",%.2f,%d,%d]", allMedicines->allMedList[i].getBarcode(), allMedicines->allMedList[i].getDescription(),
                                  allMedicines->allMedList[i].getQty() / 100.0, allMedicines->allMedList[i].getInitialQty() / 100, allMedicines->allMedList[i].getType());
            }
        }
        target += sprintf(target, "]}");
        /*target += sprintf(target, "{\"msg_type\": %d, \"Med\":", msg->msgType);
     target += sprintf(target, "[\"%s\",\"%s\", %.2f,%.2f,%d]}", msg->barcode.c_str(),  msg->name.c_str(),
     msg->qty,  msg->initQty, msg->type);*/
    }

    // return output;
    // printf("%s", target);
    // fflush(stdout);
}

void mqttTask(void *pvParameters)
{

    mqttStruct_t mqttReceive;
    while (1)
    {
        if (client.isConnected())

        {

            if (xQueueReceive(mqttQ, (void *)&mqttReceive,
                              (portTickType)portMAX_DELAY))
            {
                char payload[4096] = {0};
                structToJson(&mqttReceive, payload);

                // String p = structToJson(&mqttReceive);

                // Log.verbose("send Serialized Data %S" CR, p.c_str());
                Log.verbose("send Serialized Data %S" CR, payload);
                // client.publish(topicName + "/TX", p);
                client.publish(topicName + "/TX", payload);
                // vTaskDelay(pdMS_TO_TICKS(100));
                // client.publish("SmartCabinet/12345/TX", p);

                // esp_mqtt_client_publish(client, "SmartCabinet/12345/TX",
                //                         payload, 0, 1, 0);
                // break;
            }
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}

void sendInventoryToMqtt()
{
    Log.error("Sending inventory" CR);
    mqttStruct_t inv;
    inv.msgType = INVENTORY;
    xQueueSendToBack(mqttQ, &inv, pdMS_TO_TICKS(500));
    /*   for (unsigned int i = 0; i < allMedicines->getCounter() ; ++i)
    {

           mqttStruct_t inv;
    inv.msgType=INVENTORY;
          Log.verbose("Barcode %s:" CR,  allMedicines->allMedList[i].getBarcode());
            inv.barcode=allMedicines->allMedList[i].getBarcode();
            Log.verbose("Barcode %s:" CR,  inv.barcode.c_str());
            inv.name=allMedicines->allMedList[i].getDescription();
            inv.qty=allMedicines->allMedList[i].getQty()/100.0;
            inv.initQty=allMedicines->allMedList[i].getInitialQty()/100.0;
            inv.type=allMedicines->allMedList[i].getType();
           xQueueSendToBack(mqttQ, &inv, pdMS_TO_TICKS(500));
               vTaskDelay(1500);

    }*/
}
void createMqttTask()
{
    xTaskCreate(mqttTask, "mqttTask", 8192, NULL, 5, NULL);
}

void onConnectionEstablished()
{
    Log.verbose("MQTT COnnected" CR);
    // Subscribe to "mytopic/test" and display received message to Serial
    client.subscribe(topicName + "/RX", [](const String &payload) {
        // Log.verbose("Payload: %s" CR, payload);
        // const size_t capacity = 3*JSON_ARRAY_SIZE(6) + JSON_OBJECT_SIZE(4) + 350;       //3 arrays(morning, after noon, night) 6 med per array
        const size_t capacity = JSON_OBJECT_SIZE(4) + 150;
       
        DynamicJsonDocument doc(capacity);
        deserializeJson(doc, payload);

        uint8_t msgType = doc["msg_type"];
        // Serial.println (msgType,DEC);
        if (msgType == MED_DETAILS)
        {
            const char *barcode = doc["barcode"];
            const char *name = doc["name"];
            const char *type = doc["type"];
            if (scannedMed == TAKE_MED_SCANNED && cancelScan == false)
            {
                lcdSendCommand("TakeMed.t_barcode.txt=\"" + (String)barcode + "\"");
                //  vTaskDelay(pdMS_TO_TICKS(50));

                if ((String)type == "pills")
                {

                    // lcdSendCommand("TakeMed.t_type.txt=\"Dosage in pills\"");
                    //  vTaskDelay(pdMS_TO_TICKS(50));
                    // lcdSendCommand("v_inc.val=25");
                    lcdSendCommand("TakeMed.v_type.val=0");
                }
                else
                {
                    // lcdSendCommand("TakeMed.t_type.txt=\"Dosage in ml\"");
                    //  vTaskDelay(pdMS_TO_TICKS(50));
                    //  lcdSendCommand("v_inc.val=100");
                    lcdSendCommand("TakeMed.v_type.val=1");
                }
                // vTaskDelay(pdMS_TO_TICKS(50));
                lcdSendCommand("TakeMed.v_name.txt=\"" + (String)name + "\"");
                //  vTaskDelay(pdMS_TO_TICKS(50));
                lcdSendCommand("page TakeMed");
            }
            else if (scannedMed == PUT_MED_SCANNED && cancelScan == false)
            {
                if ((String)name == "N/A")
                {
                    lcdSendCommand("page MedNotFound");
                }
                else
                {
                    lcdSendCommand("ExpDate.t_name.txt=\"" + (String)name + "\"");
                    lcdSendCommand("page ExpDate");
                }
            }
        }
        else if (msgType == PHONE_CONNECTED)
        {
            // Serial.println("Here");
            monitoringQueueAdd(MOBILE_CONNECTED_EVT);
        }
    });

    mqttStruct_t con;
    con.msgType = CONNECTED; //msgType
    xQueueSendToBack(mqttQ, &con, pdMS_TO_TICKS(500));

    sendInventoryToMqtt();

    // client.publish(topicName, "Success", true);
}
