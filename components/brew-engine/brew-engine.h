/*
 * esp-brew-engine
 * Copyright (C) Dekien Jeroen 2024
 *
 */
#ifndef MAIN_BrewEngine_H_
#define MAIN_BrewEngine_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include <esp_http_server.h>
#include "esp_ota_ops.h"
#include "driver/gpio.h"

#include <iostream>
#include <string>
#include <sstream>
#include <functional> //function pass
#include <chrono>
#include <iomanip>
#include <ranges>
#include <map>
#include <vector>

#include "onewire_bus.h"
#include "ds18b20.h"

#include "mqtt_client.h"

#include "pidController.hpp"

#include "heater.h"

#include "mash-schedule.h"
#include "execution-step.h"
#include "temperature-sensor.h"
#include "notification.h"

#include "settings-manager.h"

#include "nlohmann_json.hpp"

#define ONEWIRE_MAX_DS18B20 10

enum TemperatureScale
{
    Celsius = 0,
    Fahrenheit = 1
};

using namespace std;
using namespace std::chrono;
using std::cout;
using std::endl;
using json = nlohmann::json;

class BrewEngine
{
private:
    static void readLoop(void *arg);
    static void pidLoop(void *arg);
    static void outputLoop(void *arg);
    static void controlLoop(void *arg);
    static void stirLoop(void *arg);
    static void reboot(void *arg);
    static void factoryReset(void *arg);
    static void buzzer(void *arg);

    void readTempSensorSettings();
    void detectOnewireTemperatureSensors();
    void initOneWire();
    void initMqtt();
    void initHeaters();
    void readSystemSettings();
    void readSettings();
    void saveMashSchedules();
    void setMashSchedule(json jSchedule);
    void savePIDSettings();
    void saveSystemSettingsJson(json config);
    void addDefaultMash();
    void start();
    void loadSchedule();
    void recalculateScheduleAfterOverTime();
    void stop();
    void logRemote(string message);
    void addDefaultHeaters();
    void readHeaterSettings();
    void saveHeaterSettings(json jHeaters);

    void saveTempSensorSettings(json data);
    void startStir(json stirConfig);
    void stopStir();
    string bootIntoRecovery();

    string processCommand(string payLoad);

    httpd_handle_t startWebserver(void);
    void stopWebserver(httpd_handle_t server);
    static esp_err_t indexGetHandler(httpd_req_t *req);
    static esp_err_t logoGetHandler(httpd_req_t *req);
    static esp_err_t manifestGetHandler(httpd_req_t *req);
    static esp_err_t otherGetHandler(httpd_req_t *req);
    static esp_err_t apiPostHandler(httpd_req_t *req);
    static esp_err_t apiOptionsHandler(httpd_req_t *req);

    // small helpers
    static string to_iso_8601(std::chrono::time_point<std::chrono::system_clock> t);

    SettingsManager *settingsManager;
    httpd_handle_t server;

    TemperatureScale temperatureScale = Celsius;
    float temperature = 0;                         // average temp, we use float beceasue ds18b20_get_temperature returns float, no point in going more percise
    float targetTemperature = 0;                   // reuqueste temp
    std::map<uint64_t, float> currentTemperatures; // map with last temp for each sensor
    std::map<time_t, int8_t> tempLog;              // integer log of averages, only used to show running history on web

    // pid
    uint8_t pidOutput = 0;
    std::optional<int8_t> manualOverrideOutput = std::nullopt;
    double kP = 10;
    double kI = 1;
    double kD = 10;
    uint16_t pidLoopTime = 60; // time in seconds for a full loop,
    bool resetPitTime = false; // bool to reset pit , we do this when out target changes
    float tempMargin = 0.5;    // we don't want to nitpick about 0.5°C, water heating is not that percise

    // execution
    bool run = false;
    bool controlRun = false;   // true when a program is running
    bool boilRun = false;      // true when a boil schedule  is running
    bool skipTempLoop = false; // When we are changing temp settings we temporarily need to skip our temp loop

    bool inOverTime = false; // when a step time isn't reached we go in overtime, we need this to know that we need recalcualtion
    string statusText = "Idle";
    std::map<string, MashSchedule *> mashSchedules;
    string selectedMashScheduleName;
    uint16_t currentMashStep;

    std::map<uint16_t, ExecutionStep *> executionSteps; // calculated real steps
    uint16_t currentExecutionStep = 0;
    uint16_t stepInterval = 60;  // calcualte a substep every x seconds
    uint16_t runningVersion = 0; // we increase our version after recalc, so client can keep uptodate with planning

    // IO
    uint8_t gpioHigh = 1;
    uint8_t gpioLow = 0;
    bool invertOutputs;

    std::vector<Heater *> heaters; // we support up to 10 heaters

    gpio_num_t oneWire_PIN;
    gpio_num_t stir_PIN;
    gpio_num_t buzzer_PIN;

    uint8_t buzzerTime; // in seconds

    std::deque<Notification *> notifications;

    string mqttUri;

    // MQTT
    bool mqttEnabled = false; // when broker is set and connection sucseeds we set this to true
    esp_mqtt_client_handle_t mqttClient;
    string mqttTopic = "";
    string mqttTopicLog = "";

    // stirring/pumping
    TaskHandle_t stirLoopHandle = NULL;
    string stirStatusText = "Idle";
    bool stirRun = false;
    uint16_t stirTimeSpan = 10; // stir timespan in minutes
    uint16_t stirIntervalStart = 0;
    uint16_t stirIntervalStop = 5;
    system_clock::time_point stirStartCycle;

    // one wire
    onewire_bus_handle_t obh;
    std::map<uint64_t, TemperatureSensor *> sensors; // map with sensor id and handle

public:
    BrewEngine(SettingsManager *settingsManager); // constructor
    void Init();

    string Hostname;

    // callbacks
    std::function<json()> GetWifiSettingsJson;
    std::function<void(json)> SaveWifiSettingsJson;
    std::function<json()> ScanWifiJson;
};

#endif /* MAIN_BrewEngine_H_ */