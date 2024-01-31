/*
 * esp-brew-engine
 * Copyright (C) Dekien Jeroen 2024
 *
 */
#include "wifi-connect-lite.h"

using namespace std;

static const char *TAG = "WiFiConnect";

WiFiConnectLite::WiFiConnectLite(SettingsManager *settingsManager)
{
    this->settingsManager = settingsManager;
}

void WiFiConnectLite::Connect()
{
    this->readSettings();

    if (this->enableAP)
    {
        ESP_LOGI(TAG, "Starting wifi Access Point");
        this->wifi_init_softap();
    }
    else
    {
        ESP_LOGI(TAG, "Starting wifi Station");
        this->wifi_init_sta();
    }
}

void WiFiConnectLite::readSettings()
{
    ESP_LOGI(TAG, "Reading Wifi Settings");

    // the logic here is that settings from nvs get preference, but if they don't exist settings from menuconfig are used and also saved to nvs
    this->ssid = this->settingsManager->Read("wifi_ssid", (string)CONFIG_WIFI_SSID);
    this->password = this->settingsManager->Read("wifi_password", (string)CONFIG_WIFI_PASS);
    this->Hostname = this->settingsManager->Read("Hostname", (string)CONFIG_HOSTNAME);

    bool configUseWifiAP = false;
// is there a cleaner way to do this?, config to bool doesn't seem to work properly
#if defined(CONFIG_WIFI_AP)
    configUseWifiAP = true;
#endif

    this->enableAP = this->settingsManager->Read("wifi_ap", (bool)configUseWifiAP);

    ESP_LOGI(TAG, "Reading Wifi Settings Done");
}

void WiFiConnectLite::saveSettings()
{
    ESP_LOGI(TAG, "Saving Wifi Settings");

    this->settingsManager->Write("wifi_ssid", this->ssid);
    this->settingsManager->Write("wifi_password", this->password);
    this->settingsManager->Write("wifi_ap", this->enableAP);
    this->settingsManager->Write("Hostname", this->Hostname);

    ESP_LOGI(TAG, "Saving Wifi Settings Done");
}

void WiFiConnectLite::gotIP(string Ip)
{
    ESP_LOGI(TAG, "Got IP:%s", Ip.c_str());
    this->Ip = Ip;

    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
}

// must be static limitation of esp-idf or freertos
void WiFiConnectLite::wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{

    // class instanse is paased as arg, due to static limitation
    WiFiConnectLite *instance = (WiFiConnectLite *)arg;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG, "Start Connect - ssid:%s password:%s ", instance->ssid.c_str(), instance->password.c_str());

        esp_wifi_connect();
    }
    // doesn't seem to work yet atm, ipv6 seems imature in esp-idf
    // else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED)
    // {
    //     ESP_LOGI(TAG, "Create Ip6 Linklocal");
    //     // esp_netif_create_ip6_linklocal(instance->sta_netif);

    //     // esp_ip6_addr_t addr;
    // }
    //   else if (event_base == IP_EVENT && event_id == IP_EVENT_GOT_IP6)
    // {
    //     ESP_LOGI(TAG, "Got Ip6");
    // }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGI(TAG, "Disconnected");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        esp_wifi_connect();
        xEventGroupClearBits(instance->s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

        string ipAddr;
        ipAddr = inet_ntoa(event->ip_info.ip);
        instance->gotIP(ipAddr);
    }
}

void WiFiConnectLite::wifi_init_sta(void)
{
    this->s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    this->sta_netif = esp_netif_create_default_wifi_sta();

    // set the Hostname
    ESP_ERROR_CHECK(esp_netif_set_hostname(this->sta_netif, this->Hostname.c_str()));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &this->wifi_event_handler, this, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &this->wifi_event_handler, this, &instance_got_ip));

    wifi_config_t wifi_config{};
    strcpy((char *)wifi_config.sta.ssid, this->ssid.c_str());
    strcpy((char *)wifi_config.sta.password, this->password.c_str());

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(this->s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "Connected to ap ssid:%s password:%s", this->ssid.c_str(), this->password.c_str());
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(TAG, "Failed to connect to ssid:%s, password:%s", this->ssid.c_str(), this->password.c_str());
    }
    else
    {
        ESP_LOGE("WifiConnect", "UNEXPECTED EVENT");
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(this->s_wifi_event_group);
}

void WiFiConnectLite::wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    this->sta_netif = esp_netif_create_default_wifi_ap();

    // set the Hostname
    ESP_ERROR_CHECK(esp_netif_set_hostname(this->sta_netif, this->Hostname.c_str()));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &this->wifi_event_handler, this, NULL));

    wifi_config_t wifi_config = {};

    strcpy((char *)wifi_config.ap.ssid, this->ssid.c_str());
    strcpy((char *)wifi_config.ap.password, this->password.c_str());
    wifi_config.ap.channel = this->apChannel;
    wifi_config.ap.max_connection = 10;
    // wifi_config.ap.pmf_cfg.required = true;
    wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;

    if (strlen(this->password.c_str()) == 0)
    {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    // future dns option inspired by?
    // https://github.com/craftmetrics/esp32-dns-server/blob/master/dns_server.c

    // other option is captive portal url via dhcp option
    // or wait unitl esp-idf implements proper dns

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wifi Access Point finished. ssid:%s password:%s channel:%d", this->ssid.c_str(), this->password.c_str(), this->apChannel);
}