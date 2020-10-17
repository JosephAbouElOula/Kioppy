
#include "Medicine.h"
#include "ArduinoLog.h"
#include "NVS\KIOPPY_NVS.h"

#define profCntKey "profCnt"

Medicine::Medicine(uint8_t ID)
{
    this->ID = ID;
    updateKeys();
    nvsReadStr(nvsBarcodeKey, this->barcode, 15);
    nvsReadStr(nvsDescKey, this->description, 35);
    nvsReadU16(nvsQtyKey, &this->qty);
    nvsReadU8(nvsTypeKey, &this->type);
    // this->barcode= nvsReadStr(nvsBarcodeKey);
    // this->description = nvsReadStr(nvsDescKey);
}

Medicine::Medicine(char *barcode, uint8_t ID)
{
    this->ID = ID;
    strcpy(this->barcode, barcode);
}

Medicine::Medicine(MedicineParams_t *medParams, uint8_t ID, bool saveToNvs)
{
    this->ID = ID;
    updateKeys();
    setBarcode(medParams->barcode, saveToNvs);
    setDescription(medParams->description, saveToNvs);
    setQty(medParams->qty, saveToNvs);
    setType(medParams->type, saveToNvs);
}

void Medicine::changeID(uint8_t newID)
{
    this->ID = ID;
    updateKeys();
}

void Medicine::updateKeys()
{
    sprintf(nvsBarcodeKey, "M%dBC", ID);
    sprintf(nvsDescKey, "M%dDC", ID);
    sprintf(nvsTypeKey, "M%dTP", ID);
    sprintf(nvsQtyKey, "M%dQT", ID);
}
void Medicine::setBarcode(char *barcode, bool saveToNvs)
{
    strcpy(this->barcode, barcode);
    if (saveToNvs)
    {
        nvsSaveStr(nvsBarcodeKey, this->barcode);
    }
}

void Medicine::setDescription(char *description, bool saveToNvs)
{
    strcpy(this->description, description);
    if (saveToNvs)
    {
        nvsSaveStr(nvsDescKey, this->description);
    }
}

void Medicine::setType(uint8_t type, bool saveToNvs)
{
    this->type = type;
    if (saveToNvs)
    {
        nvsSaveU8(nvsTypeKey, this->type);
    }
}

void Medicine::setQty(uint16_t qty, bool saveToNvs)
{
    this->qty = qty;
    if (saveToNvs)
    {
        nvsSaveU8(nvsQtyKey, this->qty);
    }
}

char *Medicine::getBarcode()
{
    return this->barcode;
}

char *Medicine::getDescription()
{
    return this->description;
}

uint8_t Medicine::getType()
{
    return this->type;
}

uint16_t Medicine::getQty()
{
    return this->qty;
}

void Medicine::getParameters(MedicineParams_t *mp)
{
    mp->type = getType();
    mp->qty = getQty();
    strcpy(mp->barcode, getBarcode());
    strcpy(mp->description, getDescription());
}

void Medicines::createNewMedicine(char *barcode)
{
    this->counter++;
    Medicine M(barcode, this->counter);
    medicinesMap.insert({M.getBarcode(), M});
}

uint8_t Medicines::createNewMedicine(MedicineParams_t *medParam) {
	this->counter++;

	Medicine M(medParam, counter, 1);
	medicinesMap.insert( {M.getBarcode(), M });
	nvsSaveU8(profCntKey, counter);
	return this->counter;
}

void Medicines::loadNewMedicine() {

	this->counter++;
//	Profile P(profileParams->name, this->counter);
	Medicine M(this->counter);
	medicinesMap.insert( {M.getBarcode(), M });
}

Medicine Medicines::getMedicineByBarcode(char* barcode) {
	return (medicinesMap.find(barcode)->second);
}

uint8_t Medicines::loadMedicines() {
	uint8_t savedMedCount = 0;
	this->counter = 0;
	nvsReadU8(profCntKey, &savedMedCount);
	/*
	 ProfileParams_t* profParams = new ProfileParams_t;
	 char nvsProfileKey[10];*/

	for (int i = 0; i < savedMedCount; i++) {
		loadNewMedicine();
	}

	return savedMedCount;

}

void Medicines::deleteMedicine(char* barcode) {
	getMedicineByBarcode(ID).~Medicines();
//	profilesMap.erase(ID);
	for (uint8_t i = ID + 1; i <= this->counter; i++) {
		getProfileById(ID).changeID(i - 1);
		profilesMap.erase(i-1);
		profilesMap.insert( { i-1, getProfileById(i) });
//		profilesMap.insert({i-1,getProfileById(ID)};
		//profilesMap.insert( { i, P });
	}
	profilesMap.erase(this->counter);
	this->counter--;
	nvsSaveU8(nvsConf.profileCounter.key, counter);
}

void Profiles::deleteAllProfiles() {
	for (int i = 1; i <= this->counter; i++) {
		getProfileById(i).~Profile();
	}
	profilesMap.clear();
	nvsSaveU8(nvsConf.profileCounter.key, 0);

}
