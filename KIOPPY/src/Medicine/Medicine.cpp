
#include "Medicine.h"
#include "ArduinoLog.h"
#include "NVS\KIOPPY_NVS.h"
#include "MQTT\MQTT.h"
uint8_t Medicines::counter = 0;
Medicine::Medicine(uint8_t ID)
{
    this->ID = ID;
    updateKeys();
    nvsReadStr(nvsBarcodeKey, this->barcode, 15);
    nvsReadStr(nvsDescKey, this->description, 35);
    nvsReadU16(nvsQtyKey, &this->qty);
    nvsReadU8(nvsTypeKey, &this->type);
    nvsReadU16(nvsInitQtyKey, &this->initQty);
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
    Log.error("Barcode Before Saving: %s" CR, medParams->barcode);
    setBarcode(medParams->barcode, saveToNvs);
    setDescription(medParams->description, saveToNvs);
    setInitialQty(medParams->qty, saveToNvs);
    setQty(medParams->qty, saveToNvs);
    setType(medParams->type, saveToNvs);
}

void Medicine::changeID(uint8_t newID)
{
    this->ID = newID;
    updateKeys();
    updateNvsValues();
}
void Medicine::updateNvsValues()
{
    nvsSaveStr(nvsBarcodeKey, this->barcode);
    nvsSaveStr(nvsDescKey, this->description);
    nvsSaveU8(nvsTypeKey, this->type);
    nvsSaveU16(nvsQtyKey, this->qty);
    nvsSaveU16(nvsInitQtyKey, this->initQty);
}
uint8_t Medicine::getId()
{
    return this->ID;
}

void Medicine::updateKeys()
{
    sprintf(nvsBarcodeKey, "M%dBC", ID);
    sprintf(nvsDescKey, "M%dDC", ID);
    sprintf(nvsTypeKey, "M%dTP", ID);
    sprintf(nvsQtyKey, "M%dQT", ID);
    sprintf(nvsInitQtyKey, "M%dIQT", ID);
}
void Medicine::setBarcode(char *barcode, bool saveToNvs)
{
    Log.error("Barcode Before Saving in fnct 1 : %s" CR, barcode);
    strcpy(this->barcode, barcode);
    Log.error("Barcode Before Saving in fnct 2 : %s" CR, this->barcode);
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
        nvsSaveU16(nvsQtyKey, this->qty);
    }
}
void Medicine::setInitialQty(uint16_t iQty, bool saveToNvs)
{
    this->initQty = iQty;
    if (saveToNvs)
    {
        nvsSaveU16(nvsInitQtyKey, this->initQty);
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

uint16_t Medicine::getInitialQty()
{
    return this->initQty;
}

void Medicine::getParameters(MedicineParams_t *mp)
{
    //TODO: initial QTY not used here
    mp->type = getType();
    mp->qty = getQty();
    strcpy(mp->barcode, getBarcode());
    strcpy(mp->description, getDescription());
}

// void Medicines::createNewMedicine(char *barcode)
// {
//     this->counter++;
//     Medicine M(barcode, this->counter);
//     medicinesMap.insert({M.getBarcode(), M});
// }

Medicines::Medicines()
{
    allMedList = (Medicine *)malloc(50 * sizeof(Medicines));
    loadMedicines();
}
uint8_t Medicines::createNewMedicine(MedicineParams_t *medParam)
{
    this->counter++;

    Medicine M(medParam, counter, 1);
    Log.error("Bfore Adding to Map %s" CR, M.getBarcode());
    // medicinesMap.insert({M.getBarcode(), M});
    // medList.push_back(M);
    allMedList[counter - 1] = M;
    nvsSaveU8(nvsConf.medCnt.key, counter);
    return this->counter;
}

void Medicines::loadNewMedicine()
{

    this->counter++;
    Medicine M(this->counter);
    Log.verbose("Loading Barcode: %s" CR, M.getBarcode());
    allMedList[counter - 1] = M;
    // medicinesMap.insert({M.getBarcode(), M});
}

// Medicine* Medicines::getMedicineByBarcode(char *barcode)
// {
//     // for (auto i = medList.begin(); i != medList.end(); ++i)
//     Log.verbose("Looking for medicine" CR);
//       for (unsigned i=0; i<this->counter ; ++i)
//     {
//         Log.verbose("Looking for medicine %d" CR, i);
//         if (strcmp(allMedList[i].getBarcode(), barcode) == 0)
//         {
//             Log.verbose("Med Found" CR);
//             return &allMedList[i];
//         }
//         Log.verbose("Check returned false" CR);
//     }
//       Log.verbose("Medicine Not Found" CR);
//     return NULL;
// }

// uint8_t Medicines::getMedicineByBarcode(char *barcode, Medicine &med)
// {
//     Log.verbose("Looking for medicine" CR);
//       for (unsigned i=0; i<medList.size(); ++i)
//     {
//         Log.verbose("Looking for medicine %d" CR, i);
//         if (strcmp(medList[i].getBarcode(), barcode) == 0)
//         {
//             Log.verbose("Med Found" CR);
//             med=medList[i];
//             return 1;
//         }
//         Log.verbose("Check returned false" CR);
//     }
//       Log.verbose("Medicine Not Found" CR);
//  return 0;
// }

void Medicines::listMedicines()
{
    Log.verbose("Listing Medicines" CR);
    // for (auto i = medList.begin(); i != medList.end(); ++i)
    for (unsigned i = 0; i < this->counter; ++i)
    {
        // Log.error("Tet CR");
        Log.error("Med %s, %s, %d, %d, %d" CR, allMedList[i].getBarcode(), allMedList[i].getDescription(), allMedList[i].getQty(), allMedList[i].getInitialQty(), allMedList[i].getType());
    }
}

void Medicines::sendInventory()
{
   
    
    for (unsigned i = 0; i < this->counter; ++i){
        
    
    // Log.verbose("Barcode %s:" CR, allMedList[i].getBarcode());
  Log.error("Med %s, %s, %d, %d, %d" CR, allMedList[i].getBarcode(), allMedList[i].getDescription(), allMedList[i].getQty(), allMedList[i].getInitialQty(), allMedList[i].getType());

    }
}
uint8_t Medicines::getCounter()
{
    return this->counter;
}

void Medicines::loadMedicines()
{
    Log.verbose("Loading Medicines");

    this->counter = 0;

    for (int i = 0; i < nvsConf.medCnt.value; i++)
    {
        loadNewMedicine();
    }
    Log.verbose("Total Loaded Medicines: %d" CR, nvsConf.medCnt.value);
}

void Medicines::deleteMedicine(char *barcode)
{

    for (int i = 0; i < this->counter; ++i)
    {
        if (strcmp(allMedList[i].getBarcode(), barcode) == 0)
        {
            allMedList[i].~Medicine();
            for (int j = i; j < this->counter; ++j)
            {
                allMedList[j].changeID(j - 1);
            }
            break;
        }
    }
    this->counter--;
    nvsSaveU8(nvsConf.medCnt.key, this->counter);
}

void Medicines::deleteMedicine(uint8_t id)
{

    allMedList[id].~Medicine();
    for (int j = id + 1; j < this->counter; ++j)
    {
        allMedList[j].changeID(j - 1);
        allMedList[j - 1] = allMedList[j];
    }

    this->counter--;
    nvsSaveU8(nvsConf.medCnt.key, this->counter);
}
void Medicines::deleteAllMedicines()
{

    //  for (auto i = medList.begin(); i != medList.end(); ++i)
    for (unsigned i = 0; i < this->counter; ++i)
    {
        allMedList[i].~Medicine();
    }
    delete[] allMedList;
    this->counter = 0;
    // allMedList.clear();
    nvsSaveU8(nvsConf.medCnt.key, 0);
}
