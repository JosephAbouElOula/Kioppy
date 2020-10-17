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

typedef struct str32Config_t{
	char key[10];
	char value[33];
}str32Config_t;

typedef struct str64Config_t{
	char key[10];
	char value[65];
}str64Config_t;


typedef struct str140Config_t{
	char key[10];
	char value[141];
}str140Config_t;

typedef struct strConfig_t{
    String key;
	String str;
}strConfig_t;

typedef struct nvsConfiguration_t{
	strConfig_t		bcdMed1;
	strConfig_t		bcdMed2;			
	strConfig_t		bcdMed3;
	strConfig_t		bcdMed4;		
	strConfig_t		bcdMed5;			
	strConfig_t		bcdMed6;
	strConfig_t		bcdMed7;
	strConfig_t		bcdMed8;
	strConfig_t		bcdMed9;			
	strConfig_t		bcdMed10;
	strConfig_t		bcdMed11;		
	strConfig_t		bcdMed12;			
	strConfig_t		bcdMed13;
	strConfig_t		bcdMed14;
	strConfig_t		bcdMed15;
	strConfig_t		bcdMed16;			
	strConfig_t		bcdMed17;
	strConfig_t		bcdMed18;		
	strConfig_t		bcdMed19;			
	strConfig_t		bcdMed20;

	strConfig_t		dscMed1;
	strConfig_t		dscMed2;			
	strConfig_t		dscMed3;
	strConfig_t		dscMed4;		
	strConfig_t		dscMed5;			
	strConfig_t		dscMed6;
	strConfig_t		dscMed7;
	strConfig_t		dscMed8;
	strConfig_t		dscMed9;			
	strConfig_t		dscMed10;
	strConfig_t		dscMed11;		
	strConfig_t		dscMed12;			
	strConfig_t		dscMed13;
	strConfig_t		dscMed14;
	strConfig_t		dscMed15;
	strConfig_t		dscMed16;			
	strConfig_t		dscMed17;
	strConfig_t		dscMed18;		
	strConfig_t		dscMed19;			
	strConfig_t		dscMed20;
    
	u8Config_t		tpeMed1;
	u8Config_t		tpeMed2;			
	u8Config_t		tpeMed3;
	u8Config_t		tpeMed4;		
	u8Config_t		tpeMed5;			
	u8Config_t		tpeMed6;
	u8Config_t		tpeMed7;
	u8Config_t		tpeMed8;
	u8Config_t		tpeMed9;			
	u8Config_t		tpeMed10;
	u8Config_t		tpeMed11;		
	u8Config_t		tpeMed12;			
	u8Config_t		tpeMed13;
	u8Config_t		tpeMed14;
	u8Config_t		tpeMed15;
	u8Config_t		tpeMed16;			
	u8Config_t		tpeMed17;
	u8Config_t		tpeMed18;		
	u8Config_t		tpeMed19;			
	u8Config_t		tpeMed20;

	u16Config_t		qtyMed1;
	u16Config_t		qtyMed2;			
	u16Config_t		qtyMed3;
	u16Config_t		qtyMed4;		
	u16Config_t		qtyMed5;			
	u16Config_t		qtyMed6;
	u16Config_t		qtyMed7;
	u16Config_t		qtyMed8;
	u16Config_t		qtyMed9;			
	u16Config_t		qtyMed10;
	u16Config_t		qtyMed11;		
	u16Config_t		qtyMed12;			
	u16Config_t		qtyMed13;
	u16Config_t		qtyMed14;
	u16Config_t		qtyMed15;
	u16Config_t		qtyMed16;			
	u16Config_t		qtyMed17;
	u16Config_t		qtyMed18;		
	u16Config_t		qtyMed19;			
	u16Config_t		qtyMed20;
}nvsConfig_t;
// nvsConfig_t nvsConf; 