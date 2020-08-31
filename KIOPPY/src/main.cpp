/* 
 *  Includes
 */
#include <ArduinoLog.h>
// #include <WiFi.h>

#include <SPI.h>
#include "Hardware\Hardware.h"
#include "DHT\myDht.h"
#include "Global.h"
#include "MQTT\MQTT.h"
#include "Nextion\Nextion.h"

#include <Preferences.h> // WiFi storage
#include "WiFi/WiFi.h"

/*
 * Defines
 */

/*
 * Global Variables
 */
QueueHandle_t monitoringQ;
QueueHandle_t mqttQ;
bool fanOn = false;

bool boolSmartConfigStop;
/*
* Local Variables
*/
const char *ssid = "hs1";
const char *wpassword = "1122334400";
const int dhtTimeOut = 30000;

const int wifiTimeout = 20000; 
bool reconnecting = false;

const u_char TEMP_MAX_THRESHOLD = 30;
const u_char TEMP_MIN_THRESHOLD = 28;

TimerHandle_t dhtTimer;
TimerHandle_t wifiReconnectTimer;
BaseType_t xHigherPriorityTaskWoken;
// EspMQTTClient client(
//     "5.196.95.208",    // MQTT Broker server ip
//     1883,              // The MQTT port, default to 1883. this line can be omitted
//     "Kioppy" // Client name that uniquely identify your device
// );

EspMQTTClient client(
    "dlink-M921-cd6e",    //SSID
    "9512410909",         //Password                
    "5.196.95.208",    // MQTT Broker server ip
    // 1883,              // The MQTT port, default to 1883. this line can be omitted
    "Kioppy" // Client name that uniquely identify your device
  );

String topicName;
bool boolStopMqtt = false;

void monitoringQueueAdd(customEvents_t event)
{
  eventStruct_t ev = {.event = event};
  xQueueSendToBack(monitoringQ, &ev, portMAX_DELAY);
}
void monitoringQueueAddFromISR(customEvents_t event)
{
  eventStruct_t ev = {.event = DOOR_EVT};
  xQueueSendToBackFromISR(monitoringQ, &ev, &xHigherPriorityTaskWoken);
}

void monitoringTask(void *pvParameters)
{
  Log.verbose("Monitoring Taks" CR);
  eventStruct_t eventReceive;
  bool sendDHTtoMqtt = false;
  int timeSent = 0; //handle door debounce msgs
  while (1)
  {
    if (xQueueReceive(monitoringQ, &eventReceive, portMAX_DELAY))
    {
      switch (eventReceive.event)
      {
      case MAX_TEMP_REACHED_EVT:
      {
        Log.verbose("Max Temp Reached" CR);
        if (!fanOn)
        {
          turnFanOn();
        }
        break;
      }
      case MIN_TEMP_REACHED_EVT:
      {
        Log.verbose("Min Temp Reached" CR);
        if (fanOn)
        {
          turnFanOff();
        }
        break;
      }
      case DHT_TIMER_EVT:
      {
        Log.verbose("DHT timer" CR);
        // TODO: UPDATE SCREEN
        sendDHTtoMqtt = !sendDHTtoMqtt;
        // lcdSendCommand("txtHum.txt=\"" + String(getHum()) + "\"");
        lcdSendCommand("HomeScreen.t_temp.txt=\"" + String(getTemp()) + "\"");
        vTaskDelay(pdMS_TO_TICKS(150));
        lcdSendCommand("HomeScreen.t_hum.txt=\"" + String(getHum()) + "\"");

        if (getTemp() >= TEMP_MAX_THRESHOLD)
        {
          // Log.verbose("MAX TEMP REACHED" CR);
          //TODO: Turn Fan On
          monitoringQueueAdd(MAX_TEMP_REACHED_EVT);
        }
        else if (getTemp() <= TEMP_MIN_THRESHOLD)
        {
          //TODO: Turn Fan On
          // Log.verbose("MIN TEMP REACEHD" CR);
          monitoringQueueAdd(MIN_TEMP_REACHED_EVT);
        }
        if (sendDHTtoMqtt)
        {
          //TODO: UPDATE MQTT
          // mqttSendMessage((mqttMsgType_t)SENSORS);
          mqttStruct_t params;
          params.msgType = SENSORS;                      //msgType
          params.humidity = getDhtData().humidity;       //Hum
          params.temperature = getDhtData().temperature; //Temp
          xQueueSendToBack(mqttQ, &params, portMAX_DELAY);
          // sendDHTtoMqtt = false;
        }

        xTimerReset(dhtTimer, 0);
        break;
      }

      case DOOR_EVT:
      {
        mqttStruct_t door;
        door.msgType = DOOR; //msgType

        vTaskDelay(pdMS_TO_TICKS(100));
        if (doorIsOpen())
        {
          Log.verbose("DOOR Open" CR);
          door.open = true;
        }
        else
        {
          Log.verbose("DOOR close" CR);
          door.open = false;
          lcdSendCommand("page page0");
        }

        if (millis() - timeSent > 1000)
        {
          xQueueSendToBack(mqttQ, &door, portMAX_DELAY);
          timeSent = millis();
        }
        // openDoor();

        doorIntEn();
        break;
      }
      case CHANGE_WIFI:
      {
        Log.verbose("Smart Config Started" CR);

        boolStopMqtt = true;
        initSmartConfig();
        boolStopMqtt = false;
        // WiFi.mode(WIFI_AP_STA);
        // WiFi.beginSmartConfig();
        // while (!WiFi.smartConfigDone())
        // {
        //   vTaskDelay(100 / portTICK_PERIOD_MS);
        //   Serial.print(".");
        // }
        // Log.verbose("Smart Config Done" CR);

        // while (WiFi.status() != WL_CONNECTED) // check till connected
        // {
        //   delay(500);
        // }
        // Log.verbose("Connected to"  CR, WiFi.SSID().c_str());

        break;
      }
      default:
        break;
      }
    }
  }
}
void IRAM_ATTR dhtTimerCallBack(TimerHandle_t dhtTimer)
{
  // Log.verbose("DHT Timer Callback" CR);
  /*eventStruct_t ev = {.event = DHT_TIMER_EVT};
  xQueueSendToBackFromISR(monitoringQ, &ev, &xHigherPriorityTaskWoken);*/
  monitoringQueueAdd(DHT_TIMER_EVT);
}

void IRAM_ATTR wifiTimerCallBack(TimerHandle_t wifiReconnectTimer)
{
reconnecting=false;
}

void printTimestamp(Print *_logOutput)
{
  char c[12];
  sprintf(c, "%10lu ", millis());
  _logOutput->print(c);
}

void setup()
{
  // put your setup code here, to run once:

  Serial.begin(115200);
  while (!Serial && !Serial.available())
  {
  }

 Log.begin(LOG_LEVEL_VERBOSE, &Serial);
  Log.setPrefix(printTimestamp);

  // Barcode_Serial.begin(9600, SERIAL_8N1, Barcode_RX, Barcode_TX);

  hardwareInit();
  monitoringQ = xQueueCreate(10, sizeof(eventStruct_t));
  mqttQ = xQueueCreate(10, sizeof(mqttStruct_t));
  dhtTimer = xTimerCreate("dhtTimer", dhtTimeOut / portTICK_PERIOD_MS, pdFALSE, (void *)0, dhtTimerCallBack);
  wifiReconnectTimer =xTimerCreate("wifi timer", wifiTimeout / portTICK_PERIOD_MS, pdFALSE, (void *)0, wifiTimerCallBack);

   Log.verbose("Setup Done!" CR);

  // // wifiInit();
  // WiFi.setAutoReconnect(true);
  // WiFi.begin(ssid, wpassword);
  // Log.verbose("Connecting to %s..." CR, ssid);
  // long i = 0;
  // while (WiFi.status() != WL_CONNECTED && i < 100)
  // {
  //   i++;
  //   vTaskDelay(pdMS_TO_TICKS(100));
  //   // delay(100);
  // }

  // if (i < 200)
  // {
  //   Log.verbose("Wifi Connected!" CR);
  // }
  topicName = "Kioppy/" + WiFi.macAddress();
  // Log.verbose("Topic %s" CR, topicName.c_str()); FC:F5:C4:05:A5:F4

  xTaskCreate(monitoringTask, "MonitoringTask", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
  createMqttTask();
  // monitoringQueueAdd(CHANGE_WIFI);

  xTimerStart(dhtTimer, 0);
  vTaskDelay(pdMS_TO_TICKS(250));
  monitoringQueueAdd(DHT_TIMER_EVT);

  // monitoringQueueAdd(CHANGE_WIFI);
}

void loop()
{
//  if (!client.isConnected() && !reconnecting){
//    Log.verbose("Reconnecting..." CR);
//    reconnecting=true;
//    WiFi.reconnect();
//    xTimerStart(wifiReconnectTimer, 0);
//  }

  if (!boolStopMqtt)
  {
    client.loop();
  }
  // if (!boolStopMqtt)
  // {

  // }

  /*
LCD_SERIAL.println("LCD Serial");
Barcode_Serial.println("Barcode Serial");
  while (Barcode_Serial.available()) {
    Serial.print(char(Barcode_Serial.read()));
  }

   float t = getDhtData().temperature;
   float h = getHum();
  Log.verbose("Temp: %F, Hum: %F" CR ,t,h);
fanOff();
delay(5000);
fanOn();
delay(5000);*/
}
