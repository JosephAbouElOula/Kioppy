
#include <Arduino.h>
#include "KIOPPY_NVS.h"
#include "nvs.h"
#include "nvs_flash.h"
nvs_handle confNvsHandle;

void nvsInit()
{
        ESP_LOGD(NVS_TAG, "nvsInit");
        esp_err_t err = nvs_flash_init();
        if (err == ESP_ERR_NVS_NO_FREE_PAGES)
        {
                ESP_LOGE(NVS_TAG, "nvsInit-ERROR initializing nvs");
                ESP_LOGE(NVS_TAG, "nvsInit-ERROR no free pages, erase flash and retry");
                // NVS partition was truncated and needs to be erased
                // Retry nvs_flash_init
                ESP_ERROR_CHECK(nvs_flash_erase());
                err = nvs_flash_init();
        }
        ESP_ERROR_CHECK(err);
}

esp_err_t nvsReadStr(char *key, char *value, size_t val_len)
{
        esp_err_t err = nvs_open("conf_storage", NVS_READONLY, &confNvsHandle);
        if (err != ESP_OK)
        {
                ESP_LOGE(NVS_TAG, "nvsReadStr-Error (%s) opening NVS handle to read", esp_err_to_name(err));
        }
        else
        {
                err = nvs_get_str(confNvsHandle, key, value, &val_len);
                if (err != ESP_OK)
                {
                        ESP_LOGE(NVS_TAG, "nvsReadStr-Error (%s) when reading", esp_err_to_name(err));
                }
                nvs_close(confNvsHandle);
        }
        return err;
}

esp_err_t nvsReadU8(char *key, uint8_t *value)
{
        esp_err_t err = nvs_open("conf_storage", NVS_READONLY, &confNvsHandle);
        if (err != ESP_OK)
        {
                ESP_LOGE(NVS_TAG, "nvsReadU8-Error (%s) opening NVS handle to read", esp_err_to_name(err));
        }
        else
        {
                err = nvs_get_u8(confNvsHandle, key, value);
                if (err != ESP_OK)
                {
                        ESP_LOGE(NVS_TAG, "nvsReadU8-Error (%s) when reading", esp_err_to_name(err));
                }
                nvs_close(confNvsHandle);
        }
        return err;
}

esp_err_t nvsReadU16(char *key, uint16_t *value)
{
        esp_err_t err = nvs_open("conf_storage", NVS_READONLY, &confNvsHandle);
        if (err != ESP_OK)
        {
                ESP_LOGE(NVS_TAG, "nvsSaveU16-Error (%s) opening NVS handle to read", esp_err_to_name(err));
        }
        else
        {
                err = nvs_get_u16(confNvsHandle, key, value);
                if (err != ESP_OK)
                {
                        ESP_LOGE(NVS_TAG, "nvsSaveU16-Error (%s) when reading", esp_err_to_name(err));
                }
                nvs_close(confNvsHandle);
        }
        return err;
}

void nvsSaveStr(char *key, char *value)
{
        esp_err_t err = nvs_open("conf_storage", NVS_READWRITE, &confNvsHandle);
        if (err != ESP_OK)
        {
                ESP_LOGE(NVS_TAG, "nvsSaveStr-Error (%s) opening NVS handle to write", esp_err_to_name(err));
        }
        else
        {
                if (nvs_set_str(confNvsHandle, key, value) != ESP_OK)
                {
                        ESP_LOGE(NVS_TAG, "nvsSaveStr-Error writing %s to %s", value, key);
                }
                if (nvs_commit(confNvsHandle) != ESP_OK)
                {
                        ESP_LOGE(NVS_TAG, "nvsSaveStr-Error commiting to handle");
                }
                nvs_close(confNvsHandle);
        }
}

void nvsSaveU8(char *key, uint8_t value)
{
        esp_err_t err = nvs_open("conf_storage", NVS_READWRITE, &confNvsHandle);
        if (err != ESP_OK)
        {
                ESP_LOGE(NVS_TAG, "nvsSaveU8-Error (%s) opening NVS handle to write", esp_err_to_name(err));
        }
        else
        {
                if (nvs_set_u8(confNvsHandle, key, value) != ESP_OK)
                {
                        ESP_LOGE(NVS_TAG, "nvsSaveU8-Error writing %d to %s", value, key);
                }
                if (nvs_commit(confNvsHandle) != ESP_OK)
                {
                        ESP_LOGE(NVS_TAG, "nvsSaveU8-Error commiting to handle");
                }
                nvs_close(confNvsHandle);
        }
}

void nvsSaveU16(char *key, uint16_t value)
{
        esp_err_t err = nvs_open("conf_storage", NVS_READWRITE, &confNvsHandle);
        if (err != ESP_OK)
        {
                ESP_LOGE(NVS_TAG, "nvsSaveU16-Error (%s) opening NVS handle to write", esp_err_to_name(err));
        }
        else
        {
                if (nvs_set_u16(confNvsHandle, key, value) != ESP_OK)
                {
                        ESP_LOGE(NVS_TAG, "nvsSaveU16-Error writing %d to %s", value, key);
                }
                if (nvs_commit(confNvsHandle) != ESP_OK)
                {
                        ESP_LOGE(NVS_TAG, "nvsSaveU16-Error commiting to handle");
                }
                nvs_close(confNvsHandle);
        }
}
