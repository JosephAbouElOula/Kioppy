#pragma once
#include "Arduino.h"
#include <unordered_map>
typedef struct MedicineParams_t
{
    char barcode[15];
    char description[35];
    uint8_t type;
    uint16_t qty;
} MedicineParams_t;

class Medicine
{
public:
    Medicine(uint8_t ID);
    Medicine(char *barcode, uint8_t ID);
    Medicine(MedicineParams_t *medParams, uint8_t ID, bool saveToNvs);
    void setBarcode(char *barcode, bool saveToNvs);
    char *getBarcode();
    void setDescription(char *description, bool saveToNvs);
    char *getDescription();
    void setType(uint8_t type, bool saveToNvs);
    uint8_t getType();
    void setQty(uint16_t qty, bool saveToNvs);
    uint16_t getQty();
    void changeID(uint8_t newID);
    void getParameters(MedicineParams_t* mp);
    uint8_t getId();

private:
    uint8_t ID;
    void updateKeys();
    void updateNvsValues();
    char nvsBarcodeKey[6];
    char nvsDescKey[6];
    char nvsTypeKey[6];
    char nvsQtyKey[6];

    char barcode[15];
    char description[35];
    uint8_t type;
    uint16_t qty;
};


class Medicines{
    public:
        uint8_t loadMedicines();
        void createNewMedicine(char *barcode);
        uint8_t createNewMedicine(MedicineParams_t *medParams);
        void loadNewMedicine();
        Medicine getMedicineByBarcode(char* barcode);
        void deleteAllMedicines();
        void deleteMedicine(char* barcode);
private: 
	std::unordered_map<char*, Medicine> medicinesMap;
	std::unordered_map<uint8_t, Medicine> medicinesIDMap;
	static uint8_t counter;
};