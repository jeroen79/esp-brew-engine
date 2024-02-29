/*
 * esp-brew-engine
 * Copyright (C) Dekien Jeroen 2024
 */
#include "settings-manager.h"

using namespace std;

static const char *TAG = "SettingsManager";

SettingsManager::SettingsManager()
{
    ESP_LOGI(TAG, "SettingsManager Construct");
}

void SettingsManager::Init()
{
    // Initialize NVS
    ESP_LOGI(TAG, "NVS partition Init: Start");
    esp_err_t partInit = nvs_flash_init();
    if (partInit == ESP_ERR_NVS_NO_FREE_PAGES || partInit == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_LOGI(TAG, "NVS partition was truncated and needs to be erased");
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        partInit = nvs_flash_init();
        ESP_ERROR_CHECK(partInit);
    }
    else if (partInit != ESP_OK)
    {
        printf("Error (%s) NVS Flash init!\n", esp_err_to_name(partInit));
    }
    ESP_LOGI(TAG, "NVS partition Init: Done");

    ESP_ERROR_CHECK(nvs_open(this->Namespace.c_str(), NVS_READWRITE, this->nvsHandle));

    nvs_stats_t nvs_stats;
    nvs_get_stats("nvs", &nvs_stats);

    ESP_LOGI(TAG, "NVS Used:%d Free:%d Total:%d", nvs_stats.used_entries, nvs_stats.free_entries, nvs_stats.total_entries);

    size_t used;
    nvs_get_used_entry_count(*this->nvsHandle, &used);

    ESP_LOGI(TAG, "NVS Used:%d", used);
}

void SettingsManager::FactoryReset()
{
    // Reset NVS
    ESP_LOGI(TAG, "FactoryReset: Start");

    ESP_ERROR_CHECK(nvs_flash_erase());
    ESP_ERROR_CHECK(nvs_flash_init());

    ESP_LOGI(TAG, "FactoryReset: Done");
}

// maby an option for the future, atm it just seems to make it more complex
// template <typename T>
// T *SettingsManager::Read(string name, T *defaultValue)
// {
//     if constexpr (std::is_integral_v<T>)
//     { // constexpr only necessary on first statement
//         ESP_LOGI(TAG, "Read Int");
//     }
//     else if (std::is_floating_point_v<T>)
//     { // automatically constexpr
//         ESP_LOGI(TAG, "Read float");
//     }
//     else if (is_string<T, std::string>)
//     { // automatically constexpr
//         ESP_LOGI(TAG, "Read string");
//     }
// }

string SettingsManager::Read(string name, string defaultValue)
{

    size_t size = 0;
    esp_err_t err = nvs_get_str(*this->nvsHandle, name.c_str(), NULL, &size);

    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGE(TAG, "Error reading Setting: %s", name.c_str());
    }

    if (size == 0)
    {
        // does not exist yet, we save the default
        this->Write(name, defaultValue);
        return defaultValue;
    }

    char *chars = (char *)malloc(size);

    err = nvs_get_str(*this->nvsHandle, name.c_str(), chars, &size);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error reading Setting: %s", name.c_str());
    }

    return (string)chars;
}

vector<uint8_t> SettingsManager::Read(string name, vector<uint8_t> defaultValue)
{

    size_t size = 0;
    esp_err_t err = nvs_get_blob(*this->nvsHandle, name.c_str(), NULL, &size);

    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGE(TAG, "Error reading Setting: %s", name.c_str());
    }

    if (size == 0)
    {
        // does not exist yet, we save the default
        this->Write(name, defaultValue);
        return defaultValue;
    }

    ESP_LOGD(TAG, "Size: %d", size);

    uint8_t blob[size];

    err = nvs_get_blob(*this->nvsHandle, name.c_str(), blob, &size);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error reading Setting: %s", name.c_str());
    }

    vector<uint8_t> v_blob = vector(blob, *(&blob + 1));

    return v_blob;
}

bool SettingsManager::Read(string name, bool defaultValue)
{
    uint8_t value = 0;
    esp_err_t err = nvs_get_u8(*this->nvsHandle, name.c_str(), &value);

    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGE(TAG, "Error reading Setting: %s", name.c_str());
    }

    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        // does not exist yet, we save the default
        this->Write(name, defaultValue);
        return defaultValue;
    }

    return (bool)value;
}

uint8_t SettingsManager::Read(string name, uint8_t defaultValue)
{
    uint8_t value = 0;
    esp_err_t err = nvs_get_u8(*this->nvsHandle, name.c_str(), &value);

    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGE(TAG, "Error reading Setting: %s", name.c_str());
    }

    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        // does not exist yet, we save the default
        this->Write(name, defaultValue);
        return defaultValue;
    }

    return value;
}

int8_t SettingsManager::Read(string name, int8_t defaultValue)
{
    int8_t value = 0;
    esp_err_t err = nvs_get_i8(*this->nvsHandle, name.c_str(), &value);

    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGE(TAG, "Error reading Setting: %s", name.c_str());
    }

    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        // does not exist yet, we save the default
        this->Write(name, defaultValue);
        return defaultValue;
    }

    return value;
}

uint16_t SettingsManager::Read(string name, uint16_t defaultValue)
{
    uint16_t value = 0;
    esp_err_t err = nvs_get_u16(*this->nvsHandle, name.c_str(), &value);

    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGE(TAG, "Error reading Setting: %s", name.c_str());
    }

    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        // does not exist yet, we save the default
        this->Write(name, defaultValue);
        return defaultValue;
    }

    return value;
}

void SettingsManager::Write(string name, string value)
{
    esp_err_t err = nvs_set_str(*this->nvsHandle, name.c_str(), value.c_str());

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error writing Setting: %s", name.c_str());
    }
}

void SettingsManager::Write(string name, vector<uint8_t> value)
{
    int size = value.size();
    uint8_t arr[size];
    copy(value.begin(), value.end(), arr);

    esp_err_t err = nvs_set_blob(*this->nvsHandle, name.c_str(), arr, size);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error writing Setting: %s", name.c_str());
    }
}

void SettingsManager::Write(string name, bool value)
{
    esp_err_t err = nvs_set_u8(*this->nvsHandle, name.c_str(), (uint8_t)value);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error writing Setting: %s", name.c_str());
    }
}

void SettingsManager::Write(string name, uint8_t value)
{
    esp_err_t err = nvs_set_u8(*this->nvsHandle, name.c_str(), value);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error writing Setting: %s", name.c_str());
    }
}

void SettingsManager::Write(string name, int8_t value)
{
    esp_err_t err = nvs_set_i8(*this->nvsHandle, name.c_str(), value);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error writing Setting: %s", name.c_str());
    }
}

void SettingsManager::Write(string name, uint16_t value)
{
    esp_err_t err = nvs_set_u16(*this->nvsHandle, name.c_str(), value);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error writing Setting: %s", name.c_str());
    }
}