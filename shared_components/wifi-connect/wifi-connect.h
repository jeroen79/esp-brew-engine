/*
 * esp-brew-engine
 * Copyright (C) Dekien Jeroen 2024
 *
 */
#ifndef MAIN_WIFICONNECT_H_
#define MAIN_WIFICONNECT_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_sntp.h"
#include "esp_event.h"
#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include <string>
#include <functional> //function pass

#include "settings-manager.h"

#include "nlohmann_json.hpp"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

using namespace std;
using json = nlohmann::json;

class WiFiConnect
{
private:
    static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
    void wifi_init_sta(void);
    void wifi_init_softap(void);
    void gotIP(string Ip);
    void printTime();
    void obtainTime();
    void readSettings();
    void saveSettings();

    EventGroupHandle_t s_wifi_event_group; // FreeRTOS event group to signal when we are connected
    int s_retry_num = 0;
    esp_netif_t *sta_netif = NULL;
    SettingsManager *settingsManager;

    string ssid = "";
    string password = "";
    int8_t maxWifiPower = 15; // seems some boards have issues at 20

    string ntpServer = "pool.ntp.org";
    uint8_t apChannel = 7;
    bool enableAP = true;

public:
    WiFiConnect(SettingsManager *settingsManager); // constructor

    void Connect();

    json GetSettingsJson();
    void SaveSettingsJson(json config);

    json Scan();

    bool setTime = true;

    std::function<string(std::string)> GotIpCallback;

    string Hostname = "";
    string Ip = "";
};

#endif /* MAIN_WIFICONNECT_H_ */