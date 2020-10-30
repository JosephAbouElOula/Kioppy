#pragma once
#include "Arduino.h"
#include <vector>
typedef struct MedicineParams_t
{
    char barcode[15];
    char description[35];
    char expDate[9];
    uint8_t type;
    uint16_t qty;
} MedicineParams_t;

class Medicine
{
public:
    // Medicine();
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
    void setInitialQty(uint16_t qty, bool saveToNvs);
    uint16_t getInitialQty();

private:
    uint8_t ID;
    void updateKeys();
    void updateNvsValues();
    char nvsBarcodeKey[6];
    char nvsDescKey[6];
    char nvsTypeKey[6];
    char nvsQtyKey[6];
    char nvsInitQtyKey[8];

    char barcode[15];
    char description[35];
    uint8_t type;
    uint16_t qty;
      uint16_t initQty;
};


class Medicines{
    public:
        Medicines();
        void loadMedicines();
        // void createNewMedicine(char *barcode);
        void listMedicines();
        uint8_t createNewMedicine(MedicineParams_t *medParams);
        void loadNewMedicine();
        int8_t getMedicineIndex(char *barcode);
        void deleteAllMedicines();
        void deleteMedicine(char* barcode);
        void deleteMedicine(uint8_t ID);
        // std::unordered_map<char*, Medicine> medicinesMap;
        // std::vector <Medicine> medList;
         Medicine* allMedList;
        uint8_t getCounter();
        void sendInventory();
        
private: 
    // std::vector <Medicine> medList;
    // std::unordered_map<uint8_t, Medicine> medicinesIDMap;
   
	static uint8_t counter;
};

extern Medicines *allMedicines;