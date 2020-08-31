/*
* Includes
*/
#include "DHT.h"
#include "Arduino.h"
#include "myDht.h"
#include <ArduinoLog.h>
#include "Global.h"

/*
* Defines
*/
#define DHT_PIN 17
#define DHTTYPE DHT21

/*
* Local Variables
*/
DHT dht(DHT_PIN, DHTTYPE);
dhtStruct_t dhtData;
u_char dhtPeriod;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
// const u_char TEMP_MAX_THRESHOLD = 23;
// const u_char TEMP_MIN_THRESHOLD = 22;
/*
* Local Functions
*/
void readDhtData(void *pvParameters)
{
    float tmp;
    u_char retry = 0;
    while (1)
    {

    READ_SENSOR:
        portENTER_CRITICAL(&timerMux);
        tmp = dht.readTemperature();
        portEXIT_CRITICAL(&timerMux);
        if (isnan(tmp))
        {
            Log.error("Error reading dht after %d retries!" CR, retry);
            if (retry < 3)
            {
                retry++;
                vTaskDelay(250 / portTICK_PERIOD_MS);
                goto READ_SENSOR;
            }
        }
        else
        {
            retry = 0;
            dhtData.temperature = tmp;
            Log.verbose("Temperature: %F" CR, dhtData.temperature);
            /*if (dhtData.temperature >= TEMP_MAX_THRESHOLD)
            {
                // Log.verbose("MAX TEMP REACHED" CR);
                //TODO: Turn Fan On
                monitoringQueueAdd(MAX_TEMP_REACHED_EVT);
            }
            else if (dhtData.temperature <= TEMP_MIN_THRESHOLD)
            {
                //TODO: Turn Fan On
                // Log.verbose("MIN TEMP REACEHD" CR);
                monitoringQueueAdd(MIN_TEMP_REACHED_EVT);
            }*/
        }
        portENTER_CRITICAL(&timerMux);
        tmp = dht.readHumidity();
        portEXIT_CRITICAL(&timerMux);
        if (isnan(tmp))
        {
            Log.error("Error reading dht after %d retries!" CR, retry);
            if (retry < 3)
            {
                retry++;
                vTaskDelay(250 / portTICK_PERIOD_MS);
                goto READ_SENSOR;
            }
        }
        else
        {
            retry = 0;
            dhtData.humidity = tmp;
            Log.verbose("Humidity: %F" CR, dhtData.humidity);
        }

        vTaskDelay((dhtPeriod * 1000) / portTICK_PERIOD_MS);
    }
}

void initDHT(u_char pollingPeriod)
{
    dht.begin();
    dhtPeriod = pollingPeriod;
    dhtData.humidity = 0;
    dhtData.temperature = 0;
    vTaskDelay(150 / portTICK_PERIOD_MS);
    xTaskCreate(readDhtData, "dht_Data", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
}

/*
* Global Functions
*/
dhtStruct_t getDhtData()
{
    return dhtData;
}
int getTemp()
{
    return (int) (dhtData.temperature+0.5);
}

int getHum()
{
    return (int) (dhtData.humidity+0.5);
}
