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
#include <NTPClient.h>
#include <Preferences.h> // WiFi storage
#include "WiFi/WiFi.h"

#include "time.h"
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

const int ntpTimeout = 15000;
bool reconnecting = false;

const u_char TEMP_MAX_THRESHOLD = 30;
const u_char TEMP_MIN_THRESHOLD = 28;

WiFiUDP ntpUDP;               // UDP client
NTPClient timeClient(ntpUDP); // NTP client

TimerHandle_t dhtTimer;
TimerHandle_t ntpTimer;
BaseType_t xHigherPriorityTaskWoken;
// EspMQTTClient client(
//     "5.196.95.208",    // MQTT Broker server ip
//     1883,              // The MQTT port, default to 1883. this line can be omitted
//     "Kioppy" // Client name that uniquely identify your device
// );

EspMQTTClient client(
    "dlink-M921-cd6e", //SSID
    "9512410909",      //Password
    // "5.196.95.208",    // MQTT Broker server ip (test.mosquitto.rog)
    "broker.mqtt-dashboard.com",
    // 1883,              // The MQTT port, default to 1883. this line can be omitted
    "Kioppy" // Client name that uniquely identify your device
);

String topicName;
String months[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
unsigned long epochTime;
struct tm *ptm;
int monthDay;
String currentMonthName;
int currentYear;
int currentMinute;
int currentHour;
String currentDate;
String meridiem;
String currentTime;
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

        lcdSendCommand("HomeScreen.t_temp.txt=\"" + String(getTemp()) + "\"");
        vTaskDelay(pdMS_TO_TICKS(500));
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
          lcdSendCommand("Welcome.v_door.val=1");
        }
        else
        {
          Log.verbose("DOOR close" CR);
          door.open = false;
          //lcdSendCommand("page page0");
          lcdSendCommand("Welcome.v_door.val=0");
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

      case NTP_TIMER_EVT:
      {
        if (client.isConnected())
        {
          lcdSendCommand("HomeScreen.v_wifi.val=1");
        }
        else
        {
          lcdSendCommand("HomeScreen.v_wifi.val=0");
        }
vTaskDelay(pdMS_TO_TICKS(150));
        epochTime = timeClient.getEpochTime();
        ptm = gmtime((time_t *)&epochTime);
        monthDay = ptm->tm_mday;
        currentMonthName = months[ptm->tm_mon];
        currentYear = ptm->tm_year + 1900;
        char day[2];
        sprintf(day, "%02d", monthDay);

        currentDate = String(currentMonthName) + " " + String(day) + ", " + String(currentYear);
        lcdSendCommand("HomeScreen.t_date.txt=\"" + currentDate + "\"");
        vTaskDelay(pdMS_TO_TICKS(150));
        currentHour = timeClient.getHours();
        if (currentHour < 12)
        {
          meridiem = "am";
        }
        else
        {
          meridiem = "pm";
          if (currentHour > 12)
          {
            currentHour = currentHour - 12;
          }
        }
        currentMinute = timeClient.getMinutes();
        char min[2];
        sprintf(min, "%02d", currentMinute);
        currentTime = (String)currentHour + ":" + (String)min + " " + meridiem;
        lcdSendCommand("HomeScreen.t_time.txt=\"" + currentTime + "\"");
        xTimerStart(ntpTimer, 0);

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

void IRAM_ATTR ntpTimerCallback(TimerHandle_t ntpTimer)
{
  monitoringQueueAdd(NTP_TIMER_EVT);
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
  ntpTimer = xTimerCreate("wifi timer", ntpTimeout / portTICK_PERIOD_MS, pdFALSE, (void *)0, ntpTimerCallback);

  client.enableDebuggingMessages();
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
bool ntpBgun = false;
void loop()
{
  //  if (!client.isConnected() && !reconnecting){
  //    Log.verbose("Reconnecting..." CR);
  //    reconnecting=true;
  //    WiFi.reconnect();
  //    xTimerStart(wifiReconnectTimer, 0);
  //  }
  client.loop();
  if (!ntpBgun && client.isWifiConnected())
  {
    timeClient.begin();              // init NTP
    timeClient.setTimeOffset(10800); // 0= GMT, 3600 = GMT+1
    timeClient.forceUpdate();
    ntpBgun = true;
    monitoringQueueAdd(NTP_TIMER_EVT);
    // xTimerStart(ntpTimer, 0);
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
