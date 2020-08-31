#include <Preferences.h> // WiFi storage
#include <esp_wifi.h>
#include "Arduino.h"
#include <WiFi.h>
#include "ArduinoLog.h"
#include "Nextion/Nextion.h"
#include "Global.h"

const char *rssiSSID; // NO MORE hard coded set AP, all SmartConfig
const char *password;
Preferences preferences;
String PrefSSID, PrefPassword;

void IP_info()
{
    String getSsid = WiFi.SSID();
    String getPass = WiFi.psk();

    Serial.printf("\n\n\tSSID\t%s, ", getSsid.c_str());
    Serial.printf(" dBm\n"); // printf??
    Serial.printf("\tPass:\t %s\n", getPass.c_str());
    Serial.print("\n\n\tIP address:\t");
    Serial.print(WiFi.localIP());
    Serial.print(" / ");
    Serial.println(WiFi.subnetMask());
    Serial.print("\tGateway IP:\t");
    Serial.println(WiFi.gatewayIP());
    Serial.print("\t1st DNS:\t");
    Serial.println(WiFi.dnsIP());
}
void initSmartConfig()
{
    // start LED flasher
    int loopCounter = 0;

    lcdSendCommand("page WiFi_Setup");
    WiFi.mode(WIFI_AP_STA); //Init WiFi, start SmartConfig
    Serial.printf("Entering SmartConfig\n");

    WiFi.beginSmartConfig();

    while (!WiFi.smartConfigDone() && !boolSmartConfigStop)
    {
        // flash led to indicate not configured
        //   Serial.printf( "." );
        if (loopCounter >= 40) // keep from scrolling sideways forever
        {
            loopCounter = 0;
            //  Serial.printf( "\n" );
        }
        vTaskDelay(200);
        ++loopCounter;
    }
    loopCounter = 0;

    if (!boolSmartConfigStop)
    {

        // stopped flasher here

        // Serial.printf("\nSmartConfig received.\n Waiting for WiFi\n\n");
        delay(2000);

        // while (WiFi.status() != WL_CONNECTED) // check till connected
        // {
        //     delay(500);
        // }

        int WLcount = 0;
        while (WiFi.status() != WL_CONNECTED && WLcount < 200) // can take > 100 loops depending on router settings
        {
            vTaskDelay(100 / portTICK_PERIOD_MS);
            // Serial.printf(".");
            ++WLcount;
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        if (WiFi.status() == WL_CONNECTED)
        {
            Log.verbose("WiFi Connected to %s" CR, PrefSSID.c_str());
            // initSmartConfig();

            lcdSendCommand("t1.txt=\"WiFi SSID Changed to " + WiFi.SSID() + "\"");
            lcdSendCommand("tm1.en=1");
            // return true;
        }

        //   IP_info();  // connected lets see IP info

        preferences.begin("wifi", false); // put it in storage
        preferences.putString("ssid", WiFi.SSID());
        preferences.putString("password", WiFi.psk());
        preferences.end();

        delay(300);

        // return true;
    }
    // else
    // {
    //     // return false;
    //     // wifiInit();
    // }

} // END SmartConfig()

bool wifiInit() //
{
    WiFi.mode(WIFI_AP_STA); // required to read NVR before WiFi.begin()

    // load credentials from NVR, a little RTOS code here
    wifi_config_t conf;
    esp_wifi_get_config(WIFI_IF_STA, &conf); // load wifi settings to struct comf
    rssiSSID = reinterpret_cast<const char *>(conf.sta.ssid);
    password = reinterpret_cast<const char *>(conf.sta.password);

    Serial.printf("%s\n", rssiSSID);
    Serial.printf("%s\n", password);

    // Open Preferences with "wifi" namespace. Namespace is limited to 15 chars
    preferences.begin("wifi", false);
    PrefSSID = preferences.getString("ssid", "none");         //NVS key ssid
    PrefPassword = preferences.getString("password", "none"); //NVS key password
    preferences.end();

    // if (PrefSSID == "none") // New...setup wifi
    // {
    //     initSmartConfig();
    //     // delay(3000);
    //     vTaskDelay(1000 / portTICK_PERIOD_MS);
    //     // ESP.restart(); // reboot with wifi configured
    // }

    WiFi.begin(PrefSSID.c_str(), PrefPassword.c_str());

    int WLcount = 0;
    while (WiFi.status() != WL_CONNECTED && WLcount < 200) // can take > 100 loops depending on router settings
    {
        vTaskDelay(100 / portTICK_PERIOD_MS);
        // Serial.printf(".");
        ++WLcount;
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    if (WiFi.status() == WL_CONNECTED)
    {
        Log.verbose("WiFi Connected to %s" CR, PrefSSID.c_str());

        // lcdSendCommand("t1.txt=\"WiFi SSID Changed to " + WiFi.SSID() + "\"");
        // lcdSendCommand("tm1.en=1");
        return true;
    }
    else
    {
        initSmartConfig();
        return false;
    }
}