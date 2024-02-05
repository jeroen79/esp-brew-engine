/*
 * esp-brew-engine
 * Copyright (C) Dekien Jeroen 2024
 *
 */
#ifndef INCLUDE_SETTINGSMANAGER_H
#define INCLUDE_SETTINGSMANAGER_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"

#include <vector>

#include "nvs_flash.h"
#include "nvs.h"
#include "nvs_handle.hpp"

using namespace std;

class SettingsManager
{
private:
    nvs_handle_t *nvsHandle;

public:
    SettingsManager(); // constructor
    void Init();

    // maby an option for the future, atm it just seems to make it more complex
    // template <typename T>
    // T *Read(string name, T *defaultValue);

    // we only implemented the types we use atm, might me needed to add more in the future
    string Read(string name, string defaultValue);
    vector<uint8_t> Read(string name, vector<uint8_t>);
    bool Read(string name, bool defaultValue);
    uint8_t Read(string name, uint8_t defaultValue);
    int8_t Read(string name, int8_t defaultValue);
    uint16_t Read(string name, uint16_t defaultValue);

    void Write(string name, string value);
    void Write(string name, vector<uint8_t> value);
    void Write(string name, bool value);
    void Write(string name, uint8_t value);
    void Write(string name, int8_t value);
    void Write(string name, uint16_t value);

    string Namespace = "Settings";
};

#endif /* INCLUDE_SETTINGSMANAGER_H */
