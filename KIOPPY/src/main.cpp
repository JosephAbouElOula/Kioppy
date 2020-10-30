/* 
 *  Includes
 */
#include <ArduinoLog.h>
#include <WiFi.h>

#include <SPI.h>
#include "Hardware\Hardware.h"
#include "DHT\myDht.h"
#include "Global.h"
#include "MQTT\MQTT.h"
#include "Nextion\Nextion.h"
#include <NTPClient.h>
#include <Preferences.h> // WiFi storage
#include "WiFi\WiFi.h"
#include "NVS\KIOPPY_NVS.h"
#include "Medicine\Medicine.h"
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
bool blwrEnable = false;
bool cancelScan = false;
Medicines *allMedicines = new Medicines();
/*
* Local Variables
*/
const int dhtTimeOut = 30000;
// char* wifiSSID="hs1";
// char* wifiPassword="0011223344";

const int ntpTimeout = 15000;
const int wifiTimeout = 20000;
const int mobileTimeout = 90000;

const u_char TEMP_MAX_THRESHOLD = 30;
const u_char TEMP_MIN_THRESHOLD = 28;

WiFiUDP ntpUDP;               // UDP client
NTPClient timeClient(ntpUDP); // NTP client

TimerHandle_t dhtTimer;
TimerHandle_t ntpTimer;
TimerHandle_t wifiReconnectTimer;

TimerHandle_t mobileConnectedTimer;

BaseType_t xHigherPriorityTaskWoken;
EspMQTTClient client(
    "broker.mqtt-dashboard.com", // MQTT Broker server ip
    // "5.196.95.208"
    1883,                        // The MQTT port, default to 1883. this line can be omitted
    "Kioppy"                     // Client name that uniquely identify your device
);

// EspMQTTClient client(
//     "dlink-M921-cd6e", //SSID
//     "9512410909",      //Password
//     // "5.196.95.208",    // MQTT Broker server ip (test.mosquitto.rog)
//     "broker.mqtt-dashboard.com",
//     // 1883,              // The MQTT port, default to 1883. this line can be omitted
//     "Kioppy" // Client name that uniquely identify your device
// );

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

bool wifiReconnectReady = true;
bool wifiMsgQueued = false;

void monitoringQueueAdd(customEvents_t event)
{

  eventStruct_t ev = {.event = event};
  xQueueSendToBack(monitoringQ, &ev, pdMS_TO_TICKS(500));
}
void monitoringQueueAddFromISR(customEvents_t event)
{
  eventStruct_t ev = {.event = event};
  xQueueSendToBackFromISR(monitoringQ, &ev, &xHigherPriorityTaskWoken);
}

void monitoringTask(void *pvParameters)
{
  Log.verbose("Monitoring Taks" CR);
  eventStruct_t eventReceive;
  bool sendDHTtoMqtt = true;
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
        if (!fanOn && blwrEnable == 1)
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
        vTaskDelay(pdMS_TO_TICKS(150));
        lcdSendCommand("HomeScreen.t_hum.txt=\"" + String(getHum()) + "\"");
        vTaskDelay(pdMS_TO_TICKS(150));
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
          mqttSendMessage(params);
          // xQueueSendToBack(mqttQ, &params, pdMS_TO_TICKS(500));
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
          vTaskDelay(pdMS_TO_TICKS(150));
          lcdSendCommand("Welcome.v_door.val=1");
        }
        else
        {
          Log.verbose("DOOR close" CR);
          door.open = false;
          //lcdSendCommand("page page0");
          lcdSendCommand("Welcome.v_door.val=0");
          vTaskDelay(pdMS_TO_TICKS(150));
          lcdSendCommand("Welcome.v_door.val=0");
        }

        if (millis() - timeSent > 1000)
        {
          // xQueueSendToBack(mqttQ, &door, pdMS_TO_TICKS(500));
          mqttSendMessage(door);
          timeSent = millis();
        }
        // openDoor();
        Log.verbose("Attach Interrupt" CR);
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
          vTaskDelay(pdMS_TO_TICKS(150));
          lcdSendCommand("HomeScreen.v_mobile.val=0");
          monitoringQueueAdd(WIFI_RECOONECT_EVT);
        }

        vTaskDelay(pdMS_TO_TICKS(150));
        if (client.isWifiConnected())
        {
          timeClient.forceUpdate();
        }
        epochTime = timeClient.getEpochTime();
        if (epochTime > 1599423864)
        {
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
        }
        xTimerStart(ntpTimer, 0);
        // monitoringQueueAdd(WIFI_RECOONECT_EVT);
        // WiFi.begin(SSID.c_str(),Password.c_str());
        break;
      }
      case WIFI_RECOONECT_EVT:
        if (wifiReconnectReady && !client.isConnected())
        {
          Log.verbose("Trying to reconnect" CR);
          wifiReconnectReady = false;
          xTimerStart(wifiReconnectTimer, 0);
          WiFi.begin(nvsConf.wifiSSID.value, nvsConf.wifiPassword.value);

        }
        break;

      case WIFI_BLOCKING_CONNECT_EVT:
      {
        WiFi.disconnect();
        while(WiFi.status()==WL_CONNECTED); //Wait to disconnect
        Log.verbose("WiFi Disconnected...");
        WiFi.begin(nvsConf.wifiSSID.value, nvsConf.wifiPassword.value);
        unsigned long t = millis();
          Log.verbose("Trying to Connect ...");
        while(WiFi.status()!=WL_CONNECTED && millis()-t <10000){
          //10 secs connecting
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        Log.verbose("Connected ...");
        if(WiFi.status()==WL_CONNECTED){
          lcdSendCommand("page nextPageId");
        } else {
          //Wifi Not connected 
          lcdSendCommand ("page IncPass");
        }
      }
         break;
      case MOBILE_DISCONNECTED_EVT:
        xTimerStart(mobileConnectedTimer, 0);
        lcdSendCommand("HomeScreen.v_mobile.val=0");
        vTaskDelay(pdMS_TO_TICKS(150));
        break;

      case MOBILE_CONNECTED_EVT:
        xTimerStart(mobileConnectedTimer, 0);
        lcdSendCommand("HomeScreen.v_mobile.val=1");
        vTaskDelay(pdMS_TO_TICKS(150));
        break;
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
  monitoringQueueAddFromISR(DHT_TIMER_EVT);
}

void IRAM_ATTR mobileTimerCallback(TimerHandle_t mobileConnectedTimer)
{

  monitoringQueueAddFromISR(MOBILE_DISCONNECTED_EVT);
}

void IRAM_ATTR wifiReconnectTimerCallback(TimerHandle_t wifiReconnectTimer)
{
  wifiReconnectReady = true;
  wifiMsgQueued = false;
}

void IRAM_ATTR ntpTimerCallback(TimerHandle_t ntpTimer)
{
  monitoringQueueAddFromISR(NTP_TIMER_EVT);
}

void printTimestamp(Print *_logOutput)
{
  char c[12];
  sprintf(c, "%10lu ", millis());
  _logOutput->print(c);
}

void scanWiFi()
{
    WiFi.disconnect();
        while(WiFi.status()==WL_CONNECTED); //Wait to disconnect
  Log.verbose("Scanning..." CR);
  uint8_t n = WiFi.scanNetworks();
  Log.verbose("Found %d Networks" CR, n);
  lcdSendCommand("WiFiSSID.v_ssidCnt.val=" + String(n));
  vTaskDelay(pdMS_TO_TICKS(100));
  if (n > 0)
  {
    String strSSIDs = WiFi.SSID(0);
    for (int i = 1; i < n; i++)
    {
      strSSIDs = strSSIDs + "|" + WiFi.SSID(i);
    }
    Log.verbose("Networks: %s" CR, strSSIDs.c_str());
    lcdSendCommand("WiFiSSID.v_SSID.txt=\"" + strSSIDs + "\"");
  }
  lcdSendCommand("page WiFiSSID");
}
long t;
void setup()
{
  // put your setup code here, to run once:

  Serial.begin(115200);
  while (!Serial && !Serial.available())
  {
  }

  Log.begin(LOG_LEVEL_VERBOSE, &Serial);
  Log.setPrefix(printTimestamp);

  nvsInit();
  hardwareInit();

  allMedicines->loadMedicines();
  // allMedicines->deleteAllMedicines();
  // allMedicines->listMedicines();

  // writeStringToEEPROM(6,"hs1");
  // writeStringToEEPROM(40,"1122334400");
 
  monitoringQ = xQueueCreate(10, sizeof(eventStruct_t));
  mqttQ = xQueueCreate(10, sizeof(mqttStruct_t));
  dhtTimer = xTimerCreate("dhtTimer", dhtTimeOut / portTICK_PERIOD_MS, pdFALSE, (void *)0, dhtTimerCallBack);
  ntpTimer = xTimerCreate("NTP timer", ntpTimeout / portTICK_PERIOD_MS, pdFALSE, (void *)0, ntpTimerCallback);
  wifiReconnectTimer = xTimerCreate("WiFi Timer", wifiTimeout / portTICK_PERIOD_MS, pdFALSE, (void *)0, wifiReconnectTimerCallback);
  mobileConnectedTimer = xTimerCreate("Mobile Timer", mobileTimeout / portTICK_PERIOD_MS, pdFALSE, (void *)0, mobileTimerCallback);

 
  client.enableDebuggingMessages();
  Log.verbose("Setup Done!" CR);
  // Serial.println(wifiPassword);
  // Serial.println(wifiPassword);
  WiFi.begin(nvsConf.wifiSSID.value, nvsConf.wifiPassword.value);
  

  // WiFi.begin("hs1", "1122334400");
  topicName = "Kioppy/" + WiFi.macAddress();

  xTaskCreate(monitoringTask, "MonitoringTask", configMINIMAL_STACK_SIZE * 6, NULL, 5, NULL);
  createMqttTask();

  xTimerStart(dhtTimer, 0);
  vTaskDelay(pdMS_TO_TICKS(250));
  monitoringQueueAdd(DHT_TIMER_EVT);
  t=millis();
}
bool ntpBgun = false;

void loop()
{

  client.loop();

  if (!ntpBgun && client.isWifiConnected())
  {
    timeClient.begin();              // init NTP
    timeClient.setTimeOffset(7200); // 0= GMT, 3600 = GMT+1
    timeClient.forceUpdate();
    ntpBgun = true;
    monitoringQueueAdd(NTP_TIMER_EVT);
    // xTimerStart(ntpTimer, 0);
  }
  if((millis()-t)>10000)
{
t=millis();
  //  allMedicines->listMedicines();
  }
  if (!client.isConnected() && wifiReconnectReady && !wifiMsgQueued)
  {
    wifiMsgQueued = true;
    monitoringQueueAdd(WIFI_RECOONECT_EVT);
  }
}
