#pragma once
#include "../Global.h"

esp_err_t nvsReadStr(char* key, char* value, size_t val_len);
esp_err_t nvsReadU16(char* key, uint16_t* value);
esp_err_t nvsReadU8(char* key, uint8_t* value);
void nvsSaveStr(char* key, char* value);
void nvsSaveU8(char* key, uint8_t value);
void nvsSaveU16(char* key, uint16_t value);

typedef struct u8Config_t{
	String key;
	uint8_t value;
}u8Config_t;

typedef struct u16Config_t{
	String key;
	uint16_t value;
}u16Config_t;

typedef struct u32Config_t{
	char key[10];
	uint32_t value;
}u32Config_t;

typedef struct str4Config_t{
	char key[10];
	char value[5];
}str32Config_t;

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
	str32Config_t		wifiPassword;	
	str4Config_t		lockCode;
	u8Config_t			medCnt;

}nvsConfig_t;
// nvsConfig_t nvsConf; 