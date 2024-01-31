/*
 * esp-brew-engine
 * Copyright (C) Dekien Jeroen 2024
 *
 */
#include "esp_log.h"
#include "driver/gpio.h"

#include "hal/efuse_hal.h"

#include "wifi-connect.h"
#include "brew-engine.h"
#include "settings-manager.h"

using std::cout;
using std::endl;
using std::runtime_error;

#define OneWire_IO (gpio_num_t) CONFIG_OneWire /* gpio number for one wire */
#define Heat1_IO (gpio_num_t) CONFIG_HEAT1     /* gpio number for heater 1  */
#define Heat2_IO (gpio_num_t) CONFIG_HEAT2     /* gpio number for heater 2 */
#define Stir_IO (gpio_num_t) CONFIG_STIR       /* gpio number for stir */
#define MQTT_URI CONFIG_MQTT_URI

WiFiConnect *wifiConnector;
string MacAddress;

BrewEngine *brewEngine;
SettingsManager *settingsManager;

static const char *TAG = "Main";

/* Inside .cpp file, app_main function must be declared with C linkage */
extern "C" void app_main(void)
{
        cout << "BrewEngine booting up!" << endl;

        try
        {

                // We get the mac address from the efuse, this is our unique idenfitier
                uint8_t macArray[6];
                char macChars[18] = {0};
                efuse_hal_get_mac(macArray);
                sprintf(macChars, "%X:%X:%X:%X:%X:%X", macArray[0], macArray[1], macArray[2], macArray[3], macArray[4], macArray[5]);
                string macAddress = macChars;
                ESP_LOGI(TAG, "MacAddress: %s", macChars);

                // Our settings manager is helper for the nvs_get/write
                settingsManager = new SettingsManager();
                settingsManager->Init();

                // WifiConnect is a helper that connects to wifi or starts a wifi ap
                wifiConnector = new WiFiConnect(settingsManager);
                wifiConnector->Connect();

                // We use the webinterface of our app to save the wifi settings, so we need to pipe the get/save functions through
                auto fpGetSettingsJson = std::bind(&WiFiConnect::GetSettingsJson, wifiConnector);
                auto fpSaveSettingsJson = std::bind(&WiFiConnect::SaveSettingsJson, wifiConnector, std::placeholders::_1);

                // BrewEngine is the main component to regulate all brewing stuff
                brewEngine = new BrewEngine(settingsManager);
                brewEngine->Hostname = wifiConnector->Hostname;
                brewEngine->GetWifiSettingsJson = fpGetSettingsJson;
                brewEngine->SaveWifiSettingsJson = fpSaveSettingsJson;
                brewEngine->Init();

                ESP_LOGI(TAG, "BrewEngine booted");
        }
        catch (const runtime_error &e)
        {
                cout << "Exception caught: " << e.what() << endl;
        }

        cout << "app_main done" << endl;
}
