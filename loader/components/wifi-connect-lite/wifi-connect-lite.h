/*
 * esp-brew-engine
 * Copyright (C) Dekien Jeroen 2024
 *
 */
#ifndef MAIN_WIFICONNECT_LITE_H_
#define MAIN_WIFICONNECT_LITE_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"

#include <string>
#include "settings-manager.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

using namespace std;

class WiFiConnectLite
{
private:
    static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
    void wifi_init_sta(void);
    void wifi_init_softap(void);

    void gotIP(string Ip);
    void readSettings();
    void saveSettings();

    EventGroupHandle_t s_wifi_event_group; // FreeRTOS event group to signal when we are connected
    int s_retry_num = 0;
    esp_netif_t *sta_netif = NULL;
    SettingsManager *settingsManager;

    string ssid = "";
    string password = "";
    int8_t maxWifiPower = 15; // seems some boards have issues at 20

    uint8_t apChannel = 7;
    bool enableAP = true;

public:
    WiFiConnectLite(SettingsManager *settingsManager); // constructor

    void Connect();

    string Hostname = "";
    string Ip = "";
};

#endif /* MAIN_WIFICONNECT_LITE_H_ */