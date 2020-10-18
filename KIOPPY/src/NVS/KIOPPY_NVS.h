#pragma once
#include "../Global.h"


typedef struct u8Config_t{
	char key[10];
	uint8_t value;
}u8Config_t;

typedef struct u16Config_t{
	char key[10];
	uint16_t value;
}u16Config_t;

typedef struct u32Config_t{
	char key[10];
	uint32_t value;
}u32Config_t;

typedef struct str4Config_t{
	char key[10];
	char value[5];
}strConfig_t;

typedef struct str32Config_t{
	char key[10];
	char value[33];
}str32Config_t;

typedef struct str64Config_t{
	char key[10];
	char value[65];
}str64Config_t;

typedef struct nvsConfiguration_t{
	str32Config_t		wifiSSID;
	str64Config_t		wifiPassword;	
	str4Config_t		lockCode;
	u8Config_t			medCnt;
	u8Config_t			blwr;
	u8Config_t			needs_setup;

}nvsConfig_t;

extern nvsConfig_t nvsConf;
esp_err_t nvsReadStr(char* key, char* value, size_t val_len);
esp_err_t nvsReadU16(char* key, uint16_t* value);
esp_err_t nvsReadU8(char* key, uint8_t* value);

void nvsSaveStr(char* key, const char* value);
void nvsSaveU8(char* key, uint8_t value);
void nvsSaveU16(char* key, uint16_t value);
void nvsInit();
void nvsGetConf(nvsConfig_t* conf);