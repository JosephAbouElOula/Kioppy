#include <Preferences.h> // WiFi storage
#include <esp_wifi.h>
#include "Arduino.h"
#include <WiFi.h>
#include "ArduinoLog.h"
#include "Nextion/Nextion.h"
#include "Global.h"


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