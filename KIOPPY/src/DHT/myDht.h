#pragma once
#include "Arduino.h"

void initDHT(u_char pollingPeriod);
typedef struct {
	float temperature;
	float humidity;
} dhtStruct_t;


dhtStruct_t getDhtData();
int getHum();
int getTemp();