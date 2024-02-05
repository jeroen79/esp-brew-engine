/*
 * esp-brew-engine
 * Copyright (C) Dekien Jeroen 2024
 */
#include "brew-engine.h"

using namespace std;
using json = nlohmann::json;

static const char *TAG = "BrewEngine";

// esp http server only works with static handlers, no other option atm then to save a pointer.
BrewEngine *mainInstance;

BrewEngine::BrewEngine(SettingsManager *settingsManager)
{
	ESP_LOGI(TAG, "BrewEngine Construct");
	this->settingsManager = settingsManager;
	mainInstance = this;
}

void BrewEngine::Init()
{
	// read the post important settings first so when van set outputs asap.
	this->readSystemSettings();

	// init gpio as soon as possible
	if (this->invertOutputs)
	{
		this->gpioHigh = 0;
		this->gpioLow = 1;
	}

	this->heaterCount = 0;

	// detect heater count and set the outputs
	if (this->heat1_PIN)
	{
		ESP_LOGI(TAG, "Heater 1 Configured");
		this->heaterCount = 1;
		gpio_reset_pin(this->heat1_PIN);
		gpio_set_direction(this->heat1_PIN, GPIO_MODE_OUTPUT);
		gpio_set_level(this->heat1_PIN, this->gpioLow);

		if (this->heat2_PIN)
		{
			ESP_LOGI(TAG, "Heater 2 Configured");
			this->heaterCount = 2;
			gpio_reset_pin(this->heat2_PIN);
			gpio_set_direction(this->heat2_PIN, GPIO_MODE_OUTPUT);
			gpio_set_level(this->heat2_PIN, this->gpioLow);
		}
	}
	else
	{
		ESP_LOGW(TAG, "You need to configure at least 1 heater");
	}

	// init heater vector and set to false
	for (int i = 0; i < heaterCount; i++)
	{
		this->heaterOn.push_back(false);
	}

	if (!this->stir_PIN)
	{
		ESP_LOGW(TAG, "StirPin is not configured!");

		this->stirStatusText = "Disabled";
	}
	else
	{
		gpio_reset_pin(this->stir_PIN);
		gpio_set_direction(this->stir_PIN, GPIO_MODE_OUTPUT);
		gpio_set_level(this->stir_PIN, this->gpioLow);
	}

	// read other settings like maishschedules and pid
	this->readSettings();

	this->readTempSensorSettings();

	this->initOneWire();

	this->detectOnewireTemperatureSensors();

	this->initMqtt();

	this->run = true;

	xTaskCreate(&this->readLoop, "readloop_task", 4096, this, 5, NULL);

	this->server = this->startWebserver();
}

void BrewEngine::readSystemSettings()
{
	ESP_LOGI(TAG, "Reading BrewEngine Settings");

	// io settings
	this->oneWire_PIN = (gpio_num_t)this->settingsManager->Read("onewirePin", (uint16_t)CONFIG_OneWire);
	this->heat1_PIN = (gpio_num_t)this->settingsManager->Read("heat1Pin", (uint16_t)CONFIG_HEAT1);
	this->heat2_PIN = (gpio_num_t)this->settingsManager->Read("heat2Pin", (uint16_t)CONFIG_HEAT2);
	this->stir_PIN = (gpio_num_t)this->settingsManager->Read("stirPin", (uint16_t)CONFIG_STIR);

	bool configInvertOutputs = false;
// is there a cleaner way to do this?, config to bool doesn't seem to work properly
#if defined(CONFIG_InvertOutputs)
	configInvertOutputs = true;
#endif
	this->invertOutputs = this->settingsManager->Read("invertOutputs", configInvertOutputs);

	// mqtt
	this->mqttUri = this->settingsManager->Read("mqttUri", (string)CONFIG_MQTT_URI);

	ESP_LOGI(TAG, "Reading BrewEngine Settings Done");
}

void BrewEngine::saveSystemSettingsJson(json config)
{
	ESP_LOGI(TAG, "Saving System Settings");

	if (!config["onewirePin"].is_null() && config["onewirePin"].is_number())
	{
		this->settingsManager->Write("onewirePin", (uint16_t)config["onewirePin"]);
		this->oneWire_PIN = (gpio_num_t)config["onewirePin"];
	}
	if (!config["heat1Pin"].is_null() && config["heat1Pin"].is_number())
	{
		this->settingsManager->Write("heat1Pin", (uint16_t)config["heat1Pin"]);
		this->heat1_PIN = (gpio_num_t)config["heat1Pin"];
	}
	if (!config["heat2Pin"].is_null() && config["heat2Pin"].is_number())
	{
		this->settingsManager->Write("heat2Pin", (uint16_t)config["heat2Pin"]);
		this->heat2_PIN = (gpio_num_t)config["heat2Pin"];
	}
	if (!config["stirPin"].is_null() && config["stirPin"].is_number())
	{
		this->settingsManager->Write("stirPin", (uint16_t)config["stirPin"]);
		this->stir_PIN = (gpio_num_t)config["stirPin"];
	}
	if (!config["invertOutputs"].is_null() && config["invertOutputs"].is_boolean())
	{
		this->settingsManager->Write("invertOutputs", (bool)config["invertOutputs"]);
		this->invertOutputs = (bool)config["invertOutputs"];
	}
	if (!config["mqttUri"].is_null() && config["mqttUri"].is_string())
	{
		this->settingsManager->Write("mqttUri", (string)config["mqttUri"]);
		this->mqttUri = config["mqttUri"];
	}

	ESP_LOGI(TAG, "Saving System Settings Done");
}

void BrewEngine::readSettings()
{
	ESP_LOGI(TAG, "Reading BrewEngine Settings");

	vector<uint8_t> empty = json::to_msgpack(json::array({}));
	vector<uint8_t> serialized = this->settingsManager->Read("mashschedules", empty);

	this->mashSchedules.clear();
	json jSchedules = json::from_msgpack(serialized);

	if (jSchedules.empty())
	{
		ESP_LOGI(TAG, "Adding Default Mash Schedules");
		this->addDefaultMash();
		this->saveMashSchedules();
	}
	else
	{
		for (auto &el : jSchedules.items())
		{
			json jSchedule = el.value();

			auto schedule = new MashSchedule();
			schedule->from_json(jSchedule);

			this->mashSchedules.insert_or_assign(schedule->name, schedule);
		}
	}

	// we save and load pid doubles as unit16 becease nvs doesnt' have double support, and we are happy with only 1 decimal
	uint16_t pint = this->settingsManager->Read("kP", (uint16_t)(this->kP * 10));
	uint16_t iint = this->settingsManager->Read("kI", (uint16_t)(this->kI * 10));
	uint16_t dint = this->settingsManager->Read("kD", (uint16_t)(this->kD * 10));

	this->kP = (double)pint / 10;
	this->kI = (double)iint / 10;
	this->kD = (double)dint / 10;
}

void BrewEngine::saveMashSchedules()
{
	ESP_LOGI(TAG, "Saving Mash Schedules");

	json jSchedules = json::array({});
	for (auto const &[key, val] : this->mashSchedules)
	{
		json jSchedule = val->to_json();
		jSchedules.push_back(jSchedule);
	}

	// serialize to MessagePack for size
	vector<uint8_t> serialized = json::to_msgpack(jSchedules);
	this->settingsManager->Write("mashschedules", serialized);

	ESP_LOGI(TAG, "Saving Mash Schedules Done");
}

void BrewEngine::savePIDSettings()
{
	ESP_LOGI(TAG, "Saving PID Settings");

	uint16_t pint = static_cast<uint16_t>(this->kP * 10);
	uint16_t iint = static_cast<uint16_t>(this->kI * 10);
	uint16_t dint = static_cast<uint16_t>(this->kD * 10);

	ESP_LOGI(TAG, "Save Pid: %d %d %d", pint, iint, dint);

	this->settingsManager->Write("kP", pint);
	this->settingsManager->Write("kI", iint);
	this->settingsManager->Write("kD", dint);

	ESP_LOGI(TAG, "Saving PID Settings Done");
}

void BrewEngine::addDefaultMash()
{
	auto defaultMash = new MashSchedule();
	defaultMash->name = "Default";
	defaultMash->steps.clear();

	auto defaultMash_s1 = new MashStep();
	defaultMash_s1->index = 0;
	defaultMash_s1->name = "Beta Amylase";
	defaultMash_s1->temperature = 64;
	defaultMash_s1->stepTime = 0;
	defaultMash_s1->extendStepTimeIfNeeded = true;
	defaultMash_s1->time = 45;
	defaultMash->steps.push_back(defaultMash_s1);

	auto defaultMash_s2 = new MashStep();
	defaultMash_s2->index = 1;
	defaultMash_s2->name = "Alpha Amylase";
	defaultMash_s2->temperature = 72;
	defaultMash_s2->stepTime = 5;
	defaultMash_s2->extendStepTimeIfNeeded = true;
	defaultMash_s2->time = 20;
	defaultMash->steps.push_back(defaultMash_s2);

	auto defaultMash_s3 = new MashStep();
	defaultMash_s3->index = 2;
	defaultMash_s3->name = "Mash Out";
	defaultMash_s3->temperature = 78;
	defaultMash_s3->stepTime = 5;
	defaultMash_s3->extendStepTimeIfNeeded = true;
	defaultMash_s3->time = 5;
	defaultMash->steps.push_back(defaultMash_s3);

	this->mashSchedules.insert_or_assign(defaultMash->name, defaultMash);

	auto ryeMash = new MashSchedule();
	ryeMash->name = "Rye Mash";
	ryeMash->steps.clear();

	auto ryeMash_s1 = new MashStep();
	ryeMash_s1->index = 0;
	ryeMash_s1->name = "Beta Glucanase";
	ryeMash_s1->temperature = 43;
	ryeMash_s1->stepTime = 0;
	ryeMash_s1->extendStepTimeIfNeeded = true;
	ryeMash_s1->time = 20;
	ryeMash->steps.push_back(ryeMash_s1);

	auto ryeMash_s2 = new MashStep();
	ryeMash_s2->index = 1;
	ryeMash_s2->name = "Beta Amylase";
	ryeMash_s2->temperature = 64;
	ryeMash_s2->stepTime = 5;
	ryeMash_s2->extendStepTimeIfNeeded = true;
	ryeMash_s2->time = 45;
	ryeMash->steps.push_back(ryeMash_s2);

	auto ryeMash_s3 = new MashStep();
	ryeMash_s3->index = 2;
	ryeMash_s3->name = "Alpha Amylase";
	ryeMash_s3->temperature = 72;
	ryeMash_s3->stepTime = 5;
	ryeMash_s3->extendStepTimeIfNeeded = true;
	ryeMash_s3->time = 20;
	ryeMash->steps.push_back(ryeMash_s3);

	auto ryeMash_s4 = new MashStep();
	ryeMash_s4->index = 3;
	ryeMash_s4->name = "Mash Out";
	ryeMash_s4->temperature = 78;
	ryeMash_s4->stepTime = 5;
	ryeMash_s4->extendStepTimeIfNeeded = true;
	ryeMash_s4->time = 5;
	ryeMash->steps.push_back(ryeMash_s4);

	this->mashSchedules.insert_or_assign(ryeMash->name, ryeMash);
}

void BrewEngine::readTempSensorSettings()
{
	vector<uint8_t> empty = json::to_msgpack(json::array({}));
	vector<uint8_t> serialized = this->settingsManager->Read("tempsensors", empty);

	json jTempSensors = json::from_msgpack(serialized);

	for (auto &el : jTempSensors.items())
	{
		auto jSensor = el.value();

		auto sensor = new TemperatureSensor();
		sensor->from_json(jSensor);

		uint64_t sensorId = sensor->id;

		ESP_LOGI(TAG, "Sensor From Settings address: %016llX, ID:%llu", sensorId, sensorId);

		this->sensors.insert_or_assign(sensorId, sensor);
	}
}

void BrewEngine::saveTempSensorSettings(json jTempSensors)
{
	ESP_LOGI(TAG, "Saving Temp Sensor Settings");

	// update running data
	// keep running loop ok?, or do we need to stop it?

	for (auto &el : jTempSensors.items())
	{
		auto jSensor = el.value();
		string stringId = jSensor["id"].get<string>();
		uint64_t sensorId = std::stoull(stringId);

		std::map<uint64_t, TemperatureSensor *>::iterator it;
		it = this->sensors.find(sensorId);

		if (it == this->sensors.end())
		{
			// doesn't exist anymore, just ignore
			ESP_LOGI(TAG, "doesn't exist anymore, just ignore %llu", sensorId);
			continue;
		}
		else
		{
			ESP_LOGI(TAG, "update it %llu", sensorId);
			// update it
			TemperatureSensor *sensor = it->second;
			sensor->name = jSensor["name"];
			sensor->color = jSensor["color"];

			if (!jSensor["enabled"].is_null() && jSensor["enabled"].is_boolean())
			{
				sensor->enabled = jSensor["enabled"];
			}

			if (!jSensor["offset"].is_null() && jSensor["offset"].is_number())
			{
				sensor->offset = (float)jSensor["offset"];
			}
		}
	}

	// We also need to delete sensors that are no longer in the list
	for (auto const &[key, sensor] : this->sensors)
	{
		uint64_t sensorId = sensor->id;
		string stringId = to_string(sensorId); // json doesn't support unit64 so in out json id is string
		auto foundSensor = std::find_if(jTempSensors.begin(), jTempSensors.end(), [&stringId](const json &x)
										{
											auto it = x.find("id");
											return it != x.end() and it.value() == stringId; });

		// remove it
		if (foundSensor == jTempSensors.end())
		{
			ESP_LOGI(TAG, "Erasing sensor %llu", sensorId);
			this->sensors.erase(sensorId);
		}
	}

	// // Convert sensors to json and save to nvram
	json jSensors = json::array({});

	for (auto const &[key, val] : this->sensors)
	{
		json jSensor = val->to_json();
		jSensors.push_back(jSensor);
	}

	// Serialize to MessagePack for size
	vector<uint8_t> serialized = json::to_msgpack(jSensors);

	this->settingsManager->Write("tempsensors", serialized);

	ESP_LOGI(TAG, "Saving Temp Sensor Settings Done");
}

void BrewEngine::initMqtt()
{
	// return if no broker is configured
	if (this->mqttUri.find("mqtt://") == std::string::npos)
	{
		return;
	}

	ESP_LOGI(TAG, "initMqtt: Start");

	esp_mqtt_client_config_t mqtt5_cfg = {};
	mqtt5_cfg.broker.address.uri = this->mqttUri.c_str();
	mqtt5_cfg.session.protocol_ver = MQTT_PROTOCOL_V_5;
	mqtt5_cfg.network.disable_auto_reconnect = false;

	this->mqttClient = esp_mqtt_client_init(&mqtt5_cfg);
	// atm we don't need an event
	// esp_mqtt_client_register_event(this->mqttClient, MQTT_EVENT_ANY, this->mqtt_event_handler, this);
	esp_err_t err = esp_mqtt_client_start(this->mqttClient);

	if (err != ESP_OK)
	{
		ESP_LOGW(TAG, "Error Creating MQTT Client");
		return;
	}

	// string iso_datetime = this->to_iso_8601(std::chrono::system_clock::now());
	// string iso_date = iso_datetime.substr(0, 10);

	// we create a topic and just post all out data to runningLog, more complex configuration can follow in the future
	this->mqttTopic = "esp-brew-engine/" + this->Hostname + "/history";
	this->mqttTopicLog = "esp-brew-engine/" + this->Hostname + "/log";
	this->mqttEnabled = true;

	ESP_LOGI(TAG, "initMqtt: Done");
}

void BrewEngine::initOneWire()
{
	ESP_LOGI(TAG, "initOneWire: Start");

	onewire_bus_config_t bus_config;
	bus_config.bus_gpio_num = this->oneWire_PIN;

	onewire_bus_rmt_config_t rmt_config;
	rmt_config.max_rx_bytes = 10; // 1byte ROM command + 8byte ROM number + 1byte device command

	ESP_ERROR_CHECK(onewire_new_bus_rmt(&bus_config, &rmt_config, &this->obh));
	ESP_LOGI(TAG, "1-Wire bus installed on GPIO%d", this->oneWire_PIN);

	ESP_LOGI(TAG, "initOneWire: Done");
}

void BrewEngine::detectOnewireTemperatureSensors()
{

	// sensors are already loaded via json settings, but we need to add handles and status
	onewire_device_iter_handle_t iter = NULL;
	esp_err_t search_result = ESP_OK;

	// create 1-wire device iterator, which is used for device search
	ESP_ERROR_CHECK(onewire_new_device_iter(this->obh, &iter));
	ESP_LOGI(TAG, "Device iterator created, start searching...");

	int i = 0;
	do
	{

		onewire_device_t next_onewire_device = {};

		search_result = onewire_device_iter_get_next(iter, &next_onewire_device);
		if (search_result == ESP_OK)
		{ // found a new device, let's check if we can upgrade it to a DS18B20
			ds18b20_config_t ds_cfg = {};

			ds18b20_device_handle_t newHandle;

			if (ds18b20_new_device(&next_onewire_device, &ds_cfg, &newHandle) == ESP_OK)
			{
				uint64_t sensorId = next_onewire_device.address;

				ESP_LOGI(TAG, "Found a DS18B20[%d], address: %016llX ID:%llu", i, sensorId, sensorId);
				i++;

				if (this->sensors.size() >= ONEWIRE_MAX_DS18B20)
				{
					ESP_LOGI(TAG, "Max DS18B20 number reached, stop searching...");
					break;
				}

				std::map<uint64_t, TemperatureSensor *>::iterator it;
				it = this->sensors.find(sensorId);

				if (it == this->sensors.end())
				{
					ESP_LOGI(TAG, "New Sensor");

					// doesn't exist yet, we need to add it
					auto sensor = new TemperatureSensor();
					sensor->id = sensorId;
					sensor->name = to_string(sensorId);
					sensor->color = "#ffffff";
					sensor->enabled = true;
					sensor->connected = true;
					sensor->offset = 0;
					sensor->handle = newHandle;
					this->sensors.insert_or_assign(sensor->id, sensor);
				}
				else
				{
					ESP_LOGI(TAG, "Existing Sensor");
					// just set connected and handle
					TemperatureSensor *sensor = it->second;
					sensor->handle = newHandle;
					sensor->connected = true;
				}

				// set resolution for all DS18B20s
				ds18b20_set_resolution(newHandle, DS18B20_RESOLUTION_12B);
			}
			else
			{
				ESP_LOGI(TAG, "Found an unknown device, address: %016llX", next_onewire_device.address);
			}
		}
	} while (search_result != ESP_ERR_NOT_FOUND);

	ESP_ERROR_CHECK(onewire_del_device_iter(iter));
	ESP_LOGI(TAG, "Searching done, %d DS18B20 device(s) found", this->sensors.size());
}

void BrewEngine::start()
{
	// don't start if we are already running
	if (!this->controlRun)
	{
		this->controlRun = true;
		xTaskCreate(&this->pidLoop, "pidloop_task", 8192, this, 5, NULL);

		xTaskCreate(&this->outputLoop, "outputloop_task", 4096, this, 5, NULL);

		// clear old temp log
		this->tempLog.clear();

		// also clear old steps
		this->executionSteps.clear();

		if (this->selectedMashScheduleName.empty() == false)
		{
			this->loadSchedule();
			this->currentMashStep = 0;
			xTaskCreate(&this->controlLoop, "controlloop_task", 4096, this, 5, NULL);
		}

		this->statusText = "Running";
	}
}

void BrewEngine::loadSchedule()
{
	auto pos = this->mashSchedules.find(this->selectedMashScheduleName);

	if (pos == this->mashSchedules.end())
	{
		ESP_LOGE(TAG, "Program with name: %s not found!", this->selectedMashScheduleName.c_str());
		return;
	}
	auto schedule = pos->second;

	system_clock::time_point prevTime = std::chrono::system_clock::now();
	this->executionSteps.clear();
	this->currentExecutionStep = 0;
	int stepIndex = 0;

	int prevTemp = (int)this->temperature;
	// insert the current as starting point
	auto execStep0 = new ExecutionStep();
	execStep0->time = prevTime;
	execStep0->temperature = prevTemp;
	execStep0->extendIfNeeded = false;
	this->executionSteps.insert(std::make_pair(stepIndex, execStep0));

	string iso_string = this->to_iso_8601(prevTime);
	ESP_LOGI(TAG, "Time:%s, Temp:%d Extend:%d", iso_string.c_str(), prevTemp, execStep0->extendIfNeeded);

	stepIndex++;

	for (auto const &step : schedule->steps)
	{
		// a step can actualy be 2 different executions, 1 step time that needs substeps calcualted, and one fixed

		if (step->stepTime > 0)
		{

			auto stepEndTime = prevTime + minutes(step->stepTime);
			auto secondsInStep = chrono::duration_cast<chrono::seconds>(stepEndTime - prevTime).count();
			int subStepsInStep = secondsInStep / this->executionStepSeconds;

			float tempDiffPerStep = ((float)step->temperature - (float)prevTemp) / (float)subStepsInStep;

			int prevStepTemp = 0;

			for (int j = 0; j < subStepsInStep; j++)
			{
				system_clock::time_point executionStepTime = prevTime;
				executionStepTime += seconds(j * executionStepSeconds);

				float subStepTemp = (float)prevTemp + (tempDiffPerStep * ((float)j + 1));

				// insert the current as starting point
				auto execStep = new ExecutionStep();
				execStep->time = executionStepTime;
				execStep->temperature = (int)subStepTemp;
				execStep->extendIfNeeded = false;

				// set extend if needed on last step if configured
				if (j == (subStepsInStep - 1) && step->extendStepTimeIfNeeded)
				{
					execStep->extendIfNeeded = true;
				}

				//  only insert if different or if last step
				if (execStep->temperature != prevStepTemp || (j == subStepsInStep - 1))
				{
					this->executionSteps.insert(std::make_pair(stepIndex, execStep));
					prevStepTemp = execStep->temperature;
					stepIndex++;

					// Convert the time_point to an ISO 8601 string
					string iso_string = this->to_iso_8601(executionStepTime);

					ESP_LOGI(TAG, "Time:%s, Temp:%d Extend:%d", iso_string.c_str(), (int)subStepTemp, execStep->extendIfNeeded);
				}
			}

			prevTime = stepEndTime;
			prevTemp = prevStepTemp;
		}
		else
		{
			// we start in 10 seconds
			auto stepEndTime = prevTime + seconds(10);

			// go directly to temp
			auto execStep = new ExecutionStep();
			execStep->time = stepEndTime;
			execStep->temperature = (int)step->temperature;
			execStep->extendIfNeeded = step->extendStepTimeIfNeeded;

			this->executionSteps.insert(std::make_pair(stepIndex, execStep));

			stepIndex++;

			// Convert the time_point to an ISO 8601 string
			string iso_string = this->to_iso_8601(prevTime);

			ESP_LOGI(TAG, "Time:%s, Temp:%d Extend:%d", iso_string.c_str(), (int)step->temperature, execStep->extendIfNeeded);

			prevTime = stepEndTime;
			prevTemp = (int)step->temperature;
		}

		// for the hold time we just need add one point
		auto holdEndTime = prevTime + minutes(step->time);

		auto holdStep = new ExecutionStep();
		holdStep->time = holdEndTime;
		holdStep->temperature = (int)step->temperature;
		holdStep->extendIfNeeded = false;

		this->executionSteps.insert(std::make_pair(stepIndex, holdStep));
		stepIndex++;

		prevTime = holdEndTime;
		prevTemp = (int)step->temperature; // is normaly the same but this could change in futrure

		string iso_string2 = this->to_iso_8601(holdEndTime);
		ESP_LOGI(TAG, "Hold Time:%s, Temp:%d ", iso_string2.c_str(), (int)step->temperature);
	}

	// increate version so client can follow changes
	this->runningVersion++;
}

void BrewEngine::recalculateScheduleAfterOverTime()
{
	ESP_LOGI(TAG, "Recalculate Schedule after OverTime");

	int currentStepIndex = this->currentMashStep;

	auto currentPos = this->executionSteps.find(currentStepIndex);

	if (currentPos == this->executionSteps.end())
	{
		ESP_LOGE(TAG, "Steps not availible anymore");
		this->stop();
		return;
	}

	auto currentStep = currentPos->second;
	system_clock::time_point plannedEnd = currentStep->time;

	system_clock::time_point now = std::chrono::system_clock::now();
	auto extraSeconds = chrono::duration_cast<chrono::seconds>(now - plannedEnd).count();

	for (auto it = currentPos; it != this->executionSteps.end(); ++it)
	{
		auto step = it->second;
		auto newTime = step->time + seconds(extraSeconds);

		string iso_string = this->to_iso_8601(step->time);
		string iso_string2 = this->to_iso_8601(newTime);

		ESP_LOGI(TAG, "Time Changend From: %s, To:%s ", iso_string.c_str(), iso_string2.c_str());

		step->time = newTime;
	}

	// increate version so client can follow changes
	this->runningVersion++;
}

void BrewEngine::stop()
{
	this->controlRun = false;
	this->statusText = "Idle";
}

void BrewEngine::startStir(json stirConfig)
{
	if (!this->stir_PIN)
	{
		ESP_LOGW(TAG, "StirPin is not configured, ignoring startStir!");
		return;
	}

	system_clock::time_point now = std::chrono::system_clock::now();
	this->stirStartCycle = now;

	if (!stirConfig["max"].is_null() && stirConfig["max"].is_number())
	{
		this->stirTimeSpan = stirConfig["max"];
	}

	if (!stirConfig["intervalStart"].is_null() && stirConfig["intervalStart"].is_number())
	{
		this->stirIntervalStart = stirConfig["intervalStart"];
	}

	if (!stirConfig["intervalStop"].is_null() && stirConfig["intervalStop"].is_number())
	{
		this->stirIntervalStop = stirConfig["intervalStop"];
	}

	this->stirRun = true;

	xTaskCreate(&this->stirLoop, "stirloop_task", 4096, this, 10, &this->stirLoopHandle);

	this->stirStatusText = "Running";
}

void BrewEngine::stopStir()
{
	if (!this->stir_PIN)
	{
		ESP_LOGW(TAG, "StirPin is not configured, ignoring stopStir!");
		return;
	}

	this->stirRun = false;

	// stop at once
	gpio_set_level(this->stir_PIN, this->gpioLow);

	this->stirStatusText = "Idle";
}

void BrewEngine::stirLoop(void *arg)
{
	BrewEngine *instance = (BrewEngine *)arg;

	while (instance->run && instance->stirRun)
	{
		if (instance->stirIntervalStart == 0 && instance->stirIntervalStop == instance->stirTimeSpan)
		{
			// always on, just set high and wait for end
			gpio_set_level(instance->stir_PIN, instance->gpioHigh);
		}
		else
		{
			system_clock::time_point now = std::chrono::system_clock::now();

			auto startStirTime = instance->stirStartCycle + minutes(instance->stirIntervalStart);
			auto stopStirTime = instance->stirStartCycle + minutes(instance->stirIntervalStop);

			auto cycleEnd = instance->stirStartCycle + minutes(instance->stirTimeSpan);

			if (now >= startStirTime && now <= stopStirTime)
			{
				gpio_set_level(instance->stir_PIN, instance->gpioHigh);
			}
			else
			{
				gpio_set_level(instance->stir_PIN, instance->gpioLow);
			}

			// string iso_string1 = instance->to_iso_8601(now);
			// string iso_string2 = instance->to_iso_8601(startStirTime);
			// string iso_string3 = instance->to_iso_8601(stopStirTime);
			// string iso_string4 = instance->to_iso_8601(cycleEnd);

			// ESP_LOGI(TAG, "Stir Cycle Now: %s Start:%s Stop:%s CycleEnd: %s", iso_string1.c_str(), iso_string2.c_str(), iso_string3.c_str(), iso_string4.c_str());

			// start next cycle
			if (now >= cycleEnd)
			{
				instance->stirStartCycle = cycleEnd;
			}
		}

		vTaskDelay(pdMS_TO_TICKS(1000));
	}

	vTaskDelete(NULL);
}

void BrewEngine::readLoop(void *arg)
{
	BrewEngine *instance = (BrewEngine *)arg;

	int it = 0;

	while (instance->run)
	{
		int nrOfSensors = 0;
		float sum = 0.0;

		vTaskDelay(pdMS_TO_TICKS(1000));

		for (auto &[key, sensor] : instance->sensors)
		{
			float temperature;
			ds18b20_device_handle_t handle = sensor->handle;
			string stringId = std::to_string(key);

			// not enabled or connected, continue
			if (!sensor->enabled || !sensor->handle || !sensor->connected)
			{
				continue;
			}

			esp_err_t err = ds18b20_trigger_temperature_conversion(handle);

			if (err != ESP_OK)
			{
				ESP_LOGW(TAG, "Error Reading from [%s], disabling sensor!", stringId.c_str());
				sensor->connected = false;
				sensor->lastTemp = 0;
				instance->currentTemperatures.erase(key);
				continue;
			};

			err = ds18b20_get_temperature(handle, &temperature);

			if (err != ESP_OK)
			{
				ESP_LOGW(TAG, "Error Reading from [%s], disabling sensor!", stringId.c_str());
				sensor->connected = false;
				sensor->lastTemp = 0;
				instance->currentTemperatures.erase(key);
				continue;
			};

			ESP_LOGD(TAG, "temperature read from [%s]: %.2fC", stringId.c_str(), temperature);

			// todo compensate
			sensor->lastTemp = temperature;
			sum += temperature;
			nrOfSensors++;

			// we also add our temps to a map individualy, might be nice to see bottom and top temp in gui
			instance->currentTemperatures.insert_or_assign(key, sensor->lastTemp);
		}

		float avg = sum / nrOfSensors;

		ESP_LOGD(TAG, "Avg Temperature: %.2fC", avg);

		instance->temperature = avg;

		// when controlrun is true we need to keep out data
		if (instance->controlRun)
		{
			// we don't have that much ram so we log only every 5 cycles

			it++;
			if (it > 5)
			{
				it = 0;
				int lastTemp = 0;

				if (!instance->tempLog.empty())
				{
					auto lastValue = instance->tempLog.rbegin();
					lastTemp = lastValue->second;
				}

				if (lastTemp != (int)avg)
				{
					// decided agains chrono just make it a hell lot more complex
					// instance->tempLog.insert(std::make_pair(std::chrono::system_clock::now(), (int)avg));
					time_t current_raw_time = time(0);
					// System time: number of seconds since 00:00,
					instance->tempLog.insert(std::make_pair(current_raw_time, (int)avg));

					ESP_LOGI(TAG, "Logging: %dC", (int)avg);
				}
				else
				{
					ESP_LOGI(TAG, "Skip same");
				}
			}

			if (instance->mqttEnabled)
			{
				string iso_datetime = to_iso_8601(std::chrono::system_clock::now());
				json jPayload;
				jPayload["time"] = iso_datetime;
				jPayload["temp"] = instance->temperature;
				jPayload["target"] = instance->targetTemperature;
				jPayload["output"] = instance->pidOutput;
				string payload = jPayload.dump();

				esp_mqtt_client_publish(instance->mqttClient, instance->mqttTopic.c_str(), payload.c_str(), 0, 1, 1);
			}
		}
	}

	vTaskDelete(NULL);
}

void BrewEngine::pidLoop(void *arg)
{
	BrewEngine *instance = (BrewEngine *)arg;

	PIDController pid(instance->kP, instance->kI, instance->kD);
	pid.setMin(0);
	pid.setMax(100);
	pid.debug = true;

	while (instance->run && instance->controlRun)
	{

		int output = (int)pid.getOutput((double)instance->temperature, (double)instance->targetTemperature);
		instance->pidOutput = output;
		ESP_LOGI(TAG, "Pid Output: %d", instance->pidOutput);

		// manual override
		if (instance->manualOverrideOutput.has_value())
		{
			output = instance->manualOverrideOutput.value();
		}

		// init array and set all to 0
		int burn[instance->heaterCount];
		for (int j = 0; j < instance->heaterCount; j++)
		{
			burn[j] = 0;
		}

		// instance->logRemote("Pid Output " + to_string(instance->pidOutput) + "%");

		// we need to calculate our burn time per output
		for (int j = 0; j < instance->heaterCount; j++)
		{
			if (output < 0)
			{
				break;
			}

			if (output < (100 / instance->heaterCount))
			{
				// we can complete it with this heater
				burn[j] = output * instance->heaterCount;

				output = 0;

				// instance->logRemote("Pid Calc Heater " + to_string(j) + " burn: " + to_string(burn[j]) + "%");
				ESP_LOGD(TAG, "Pid Calc Heater %d: Burn: %d", j, burn[j]);
				break;
			}
			else
			{
				// we can't complete it, take out part and continue
				burn[j] = 100; // full burn
				// remove from output
				output = output - (100 / instance->heaterCount);

				// instance->logRemote("Pid Calc Heater " + to_string(j) + " burn: " + to_string(burn[j]) + "%");
				ESP_LOGD(TAG, "Pid Calc Heater %d: Burn: %d", j, burn[j]);
			}
		}

		// we keep going for the desired pidlooptime and set the burn by percent
		for (int i = 0; i < instance->pidLoopTime; i++)
		{

			if (!instance->run || !instance->controlRun)
			{
				break;
			}

			vTaskDelay(pdMS_TO_TICKS(1000));

			for (int j = 0; j < instance->heaterCount; j++)
			{
				int burnUntil = 0;

				if (burn[j] > 0)
				{
					burnUntil = ((double)burn[j] / 100) * (double)instance->pidLoopTime; // convert % back to seconds
				}

				// ESP_LOGI(TAG, "Tick j:%d burnUntil:%d", j, burnUntil);

				if (burnUntil > i)
				{									   // on
					if (instance->heaterOn[j] != true) // only when not current, we don't want to spam the logs
					{
						instance->heaterOn[j] = true;
						ESP_LOGD(TAG, "Heater %d: On", j);
						// instance->logRemote("Heater " + to_string(j) + ": On");
					}
				}
				else
				{										// off
					if (instance->heaterOn[j] != false) // only when not current, we don't want to spam the logs
					{
						instance->heaterOn[j] = false;
						ESP_LOGD(TAG, "Heater %d: Off", j);
						// instance->logRemote("Heater " + to_string(j) + ": Off");
					}
				}
			}

			// when our target changes we also update our pid target
			if (instance->resetPitTime)
			{
				instance->resetPitTime = false;
				break;
			}
		}
	}
	vTaskDelete(NULL);
}

void BrewEngine::outputLoop(void *arg)
{
	BrewEngine *instance = (BrewEngine *)arg;

	// atm we use 2 heaters fixed, maby this can be variable in the future
	gpio_set_level(instance->heat1_PIN, instance->gpioLow);
	gpio_set_level(instance->heat2_PIN, instance->gpioLow);

	while (instance->run && instance->controlRun)
	{
		vTaskDelay(pdMS_TO_TICKS(1000));

		if (instance->heaterOn[0])
		{
			ESP_LOGD(TAG, "Output1: On");
			// instance->logRemote("Output1: On");
			gpio_set_level(instance->heat1_PIN, instance->gpioHigh);
		}
		else
		{
			ESP_LOGD(TAG, "Output1: Off");
			// instance->logRemote("Output1: Off");
			gpio_set_level(instance->heat1_PIN, instance->gpioLow);
		}

		if (instance->heaterOn[1])
		{
			ESP_LOGD(TAG, "Output2: On");
			// instance->logRemote("Output2: On");
			gpio_set_level(instance->heat2_PIN, instance->gpioHigh);
		}
		else
		{
			ESP_LOGD(TAG, "Output2: Off");
			// instance->logRemote("Output2: Off");
			gpio_set_level(instance->heat2_PIN, instance->gpioLow);
		}
	}

	// set outputs off and quit thread
	gpio_set_level(instance->heat1_PIN, instance->gpioLow);
	gpio_set_level(instance->heat2_PIN, instance->gpioLow);

	vTaskDelete(NULL);
}

void BrewEngine::controlLoop(void *arg)
{
	BrewEngine *instance = (BrewEngine *)arg;

	while (instance->run && instance->controlRun)
	{

		vTaskDelay(pdMS_TO_TICKS(1000));

		system_clock::time_point now = std::chrono::system_clock::now();

		if (instance->executionSteps.size() >= instance->currentMashStep)
		{ // there are more steps
			int nextStepIndex = instance->currentMashStep;

			auto nextStep = instance->executionSteps.at(nextStepIndex);

			system_clock::time_point nextAction = nextStep->time;

			bool gotoNextStep = false;

			// always set target
			instance->targetTemperature = nextStep->temperature;

			uint secondsToGo = 0;
			// if its smaller 0 is ok!
			if (nextAction > now)
			{
				secondsToGo = chrono::duration_cast<chrono::seconds>(nextAction - now).count();
			}

			if (secondsToGo < 1)
			{ // change temp and increment Currentstep

				// string iso_string = instance->to_iso_8601(nextStep->time);
				// ESP_LOGI(TAG, "Control Time:%s, TempCur:%f, TempTarget:%d, Extend:%d, Overtime: %d", iso_string.c_str(), instance->temperature, nextStep->temperature, nextStep->extendIfNeeded, instance->inOverTime);

				if (nextStep->extendIfNeeded == true && instance->inOverTime == false && instance->temperature < nextStep->temperature)
				{
					// temp must be reached, we keep going but need to triger a recaluclation event when done
					ESP_LOGI(TAG, "OverTime Start");
					instance->logRemote("OverTime Start");
					instance->inOverTime = true;
				}
				else if (instance->inOverTime == true && instance->temperature >= nextStep->temperature)
				{
					// we reached out temp after overtime, we need to recalc the rest and start going again
					ESP_LOGI(TAG, "OverTime Done");
					instance->logRemote("OverTime Done");
					instance->inOverTime = false;
					instance->recalculateScheduleAfterOverTime();
					gotoNextStep = true;
				}
				else if (instance->inOverTime == false)
				{
					ESP_LOGI(TAG, "Going to next Step");
					gotoNextStep = true;
				}

				// else when in overtime just keep going until we reach temp
			}

			if (gotoNextStep)
			{
				instance->currentMashStep++;
				instance->resetPitTime = true;
			}
		}
		else
		{
			// last step need to stop
			ESP_LOGI(TAG, "Program Finished");
			instance->stop();
		}
	}

	vTaskDelete(NULL);
}

string BrewEngine::bootIntoRecovery()
{
	// recovery is our factory
	esp_partition_subtype_t t = ESP_PARTITION_SUBTYPE_APP_FACTORY;
	const esp_partition_t *factory_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, t, NULL);

	if (factory_partition == NULL)
	{
		ESP_LOGE(TAG, "Factory Partition not found!");
		return "Error: Factory Partition not found!";
	}

	if (esp_ota_set_boot_partition(factory_partition) == ESP_OK)
	{
		return "Device is booting into recovery hold on!";
	}

	return "Error: Unable to chage boot Partition!";
}

void BrewEngine::reboot(void *arg)
{
	vTaskDelay(2000 / portTICK_PERIOD_MS);
	esp_restart();
}

string BrewEngine::processCommand(string payLoad)
{
	ESP_LOGD(TAG, "payLoad %s", payLoad.c_str());

	json jCommand = json::parse(payLoad);
	string command = jCommand["command"];
	json data = jCommand["data"];

	ESP_LOGD(TAG, "processCommand %s", command.c_str());
	ESP_LOGD(TAG, "data %s", data.dump().c_str());

	json resultData = {};
	string message = "";
	bool success = true;

	if (command == "Data")
	{
		time_t lastLogDateTime = time(0);

		json jTempLog = json::array({});
		if (!this->tempLog.empty())
		{
			auto lastLog = this->tempLog.rend();
			lastLogDateTime = lastLog->first;

			// If we have a last date we only need to send the log increment
			if (!data["lastDate"].is_null() && data["lastDate"].is_number())
			{
				time_t lastClientDate = (time_t)data["lastDate"];
				ESP_LOGD(TAG, "lastClientDate %s", ctime(&lastClientDate));

				// most efficient seems to loop reverse and add until date is reached
				for (auto iter = this->tempLog.rbegin(); iter != this->tempLog.rend(); ++iter)
				{
					if (iter->first > lastClientDate)
					{
						json jTempLogItem;
						jTempLogItem["time"] = iter->first;
						jTempLogItem["temp"] = iter->second;
						jTempLog.push_back(jTempLogItem);
					}
					else
					{
						break;
					}
				}
			}
			else
			{
				for (auto iter = this->tempLog.rbegin(); iter != this->tempLog.rend(); ++iter)
				{
					json jTempLogItem;
					jTempLogItem["time"] = iter->first;
					jTempLogItem["temp"] = iter->second;
					jTempLog.push_back(jTempLogItem);
				}
			}
		}

		// when we have more then 1 sensor also send all current values
		json jCurrentTemps = json::array({});
		if (this->currentTemperatures.size() > 1)
		{
			for (auto const &[key, val] : this->currentTemperatures)
			{
				json jCurrentTemp;
				jCurrentTemp["sensor"] = key;
				jCurrentTemp["temp"] = (double)((int)(val * 10)) / 10; // round float to 1 digit for display
				jCurrentTemps.push_back(jCurrentTemp);
			}
		}

		resultData = {
			{"temp", (double)((int)(this->temperature * 10)) / 10}, // round float to 1 digit for display
			{"temps", jCurrentTemps},
			{"targetTemp", this->targetTemperature},
			{"output", this->pidOutput},
			{"manualOverrideOutput", nullptr},
			{"status", this->statusText},
			{"stirStatus", this->stirStatusText},
			{"lastLogDateTime", lastLogDateTime},
			{"tempLog", jTempLog},
			{"runningVersion", this->runningVersion},
		};

		if (this->manualOverrideOutput.has_value())
		{
			resultData["manualOverrideOutput"] = this->manualOverrideOutput.value();
		}
	}
	else if (command == "GetRunningSchedule")
	{
		json jRunningSchedule;
		jRunningSchedule["version"] = this->runningVersion;

		json jExecutionSteps = json::array({});
		for (auto const &[key, val] : this->executionSteps)
		{
			json jExecutionStep = val->to_json();
			jExecutionSteps.push_back(jExecutionStep);
		}
		jRunningSchedule["steps"] = jExecutionSteps;

		resultData = jRunningSchedule;
	}
	else if (command == "SetTemp")
	{

		if (data["targetTemp"].is_number())
		{
			this->targetTemperature = (float)data["targetTemp"];
		}
		else
		{
			message = "Incorrect data, integer or float expected!";
			success = false;
		}
	}
	else if (command == "SetOverrideOutput")
	{

		if (data["output"].is_null() == false && data["output"].is_number())
		{
			this->manualOverrideOutput = (int)data["output"];
		}
		else
		{
			this->manualOverrideOutput = std::nullopt;
		}

		// reset so effect is immidiate
		this->resetPitTime = true;
	}
	else if (command == "Start")
	{
		if (data["selectedMashSchedule"].is_null())
		{
			this->selectedMashScheduleName.clear();
		}
		else
		{
			this->selectedMashScheduleName = (string)data["selectedMashSchedule"];
		}

		this->start();
	}
	else if (command == "StartStir")
	{
		this->startStir(data);
	}
	else if (command == "Stop")
	{
		this->stop();
	}
	else if (command == "StopStir")
	{
		this->stopStir();
	}
	else if (command == "GetMashSchedules")
	{

		json jSchedules = json::array({});

		for (auto const &[key, val] : this->mashSchedules)
		{
			json jSchedule = val->to_json();
			jSchedules.push_back(jSchedule);
		}

		resultData = jSchedules;
	}
	else if (command == "SaveMashSchedule")
	{
		string newName = (string)data["name"];
		json newSteps = data["steps"];

		auto newMash = new MashSchedule();
		newMash->name = newName;
		newMash->steps.clear();

		for (auto &el : newSteps.items())
		{
			auto jStep = el.value();

			auto newStep = new MashStep();
			newStep->index = jStep["index"].get<uint>();
			newStep->name = jStep["name"];
			newStep->temperature = jStep["temperature"].get<int>();
			newStep->stepTime = jStep["stepTime"].get<int>();
			newStep->time = jStep["time"].get<int>();
			newStep->extendStepTimeIfNeeded = jStep["extendStepTimeIfNeeded"].get<bool>();
			newMash->steps.push_back(newStep);
		}

		newMash->sort_steps();

		this->mashSchedules.insert_or_assign(newMash->name, newMash);

		this->saveMashSchedules();
	}
	else if (command == "DeleteMashSchedule")
	{
		string deleteName = (string)data["name"];

		auto pos = this->mashSchedules.find(deleteName);

		if (pos == this->mashSchedules.end())
		{
			message = "Schedule with name: " + deleteName + " not found";
			success = false;
		}
		else
		{
			this->mashSchedules.erase(pos);
			this->saveMashSchedules();
		}
	}
	else if (command == "GetPIDSettings")
	{
		resultData = {
			{"kP", this->kP},
			{"kI", this->kI},
			{"kD", this->kD},
		};
	}
	else if (command == "SavePIDSettings")
	{
		this->kP = data["kP"].get<double>();
		this->kI = data["kI"].get<double>();
		this->kD = data["kD"].get<double>();
		this->savePIDSettings();
	}
	else if (command == "GetTempSettings")
	{
		// Convert sensors to json
		json jSensors = json::array({});

		for (auto const &[key, val] : this->sensors)
		{
			json jSensor = val->to_json();
			jSensors.push_back(jSensor);
		}

		resultData = jSensors;
	}
	else if (command == "SaveTempSettings")
	{
		this->saveTempSensorSettings(data);
	}
	else if (command == "GetWifiSettings")
	{
		// get data from wifi-connect
		if (this->GetWifiSettingsJson)
		{
			resultData = this->GetWifiSettingsJson();
		}
	}
	else if (command == "SaveWifiSettings")
	{
		// save via wifi-connect
		if (this->SaveWifiSettingsJson)
		{
			this->SaveWifiSettingsJson(data);
		}
		message = "Please restart device for changes to have effect!";
	}
	else if (command == "ScanWifi")
	{
		// scans for networks
		if (this->ScanWifiJson)
		{
			resultData = this->ScanWifiJson();
		}
	}
	else if (command == "GetSystemSettings")
	{
		resultData = {
			{"onewirePin", this->oneWire_PIN},
			{"heat1Pin", this->heat1_PIN},
			{"heat2Pin", this->heat2_PIN},
			{"stirPin", this->stir_PIN},
			{"invertOutputs", this->invertOutputs},
			{"mqttUri", this->mqttUri},
		};
	}
	else if (command == "SaveSystemSettings")
	{
		this->saveSystemSettingsJson(data);
		message = "Please restart device for changes to have effect!";
	}
	else if (command == "BootIntoRecovery")
	{
		message = this->bootIntoRecovery();

		if (message.find("Error") != std::string::npos)
		{
			success = false;
		}
		else
		{
			xTaskCreate(&this->reboot, "reboot_task", 1024, this, 5, NULL);
		}
	}

	json jResultPayload;
	jResultPayload["data"] = resultData;
	jResultPayload["success"] = success;

	if (message != "")
	{
		jResultPayload["message"] = message;
	}

	string resultPayload = jResultPayload.dump();

	return resultPayload;
}

httpd_handle_t BrewEngine::startWebserver(void)
{

	httpd_uri_t indexUri;
	indexUri.uri = "/";
	indexUri.method = HTTP_GET;
	indexUri.handler = this->indexGetHandler;

	httpd_uri_t postUri;
	postUri.uri = "/api";
	postUri.method = HTTP_POST;
	postUri.handler = this->apiPostHandler;

	httpd_uri_t optionsUri;
	optionsUri.uri = "/api";
	optionsUri.method = HTTP_OPTIONS;
	optionsUri.handler = this->apiOptionsHandler;

	httpd_uri_t otherUri;
	otherUri.uri = "/*";
	otherUri.method = HTTP_GET;
	otherUri.handler = this->otherGetHandler;

	httpd_handle_t server = NULL;
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	// whiout this the esp crashed whitout a proper warning
	config.stack_size = 20480;
	config.uri_match_fn = httpd_uri_match_wildcard;

	// Start the httpd server
	ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
	if (httpd_start(&server, &config) == ESP_OK)
	{
		// Set URI handlers
		httpd_register_uri_handler(server, &indexUri);
		httpd_register_uri_handler(server, &otherUri);
		httpd_register_uri_handler(server, &postUri);
		httpd_register_uri_handler(server, &optionsUri);
		return server;
	}

	ESP_LOGI(TAG, "Error starting server!");
	return NULL;
}

void BrewEngine::logRemote(string message)
{
	if (this->mqttEnabled)
	{
		string iso_datetime = this->to_iso_8601(std::chrono::system_clock::now());
		json jPayload;
		jPayload["time"] = iso_datetime;
		jPayload["level"] = "Debug";
		jPayload["message"] = message;
		string payload = jPayload.dump();

		esp_mqtt_client_publish(this->mqttClient, this->mqttTopicLog.c_str(), payload.c_str(), 0, 1, 1);
	}
}

void BrewEngine::stopWebserver(httpd_handle_t server)
{
	// Stop the httpd server
	httpd_stop(server);
}

esp_err_t BrewEngine::indexGetHandler(httpd_req_t *req)
{
	// ESP_LOGI(TAG, "index_get_handler");
	extern const unsigned char index_html_start[] asm("_binary_index_html_gz_start");
	extern const unsigned char index_html_end[] asm("_binary_index_html_gz_end");
	const size_t index_html_size = (index_html_end - index_html_start);
	httpd_resp_set_type(req, "text/html");
	httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
	httpd_resp_send(req, (const char *)index_html_start, index_html_size);

	return ESP_OK;
}

esp_err_t BrewEngine::otherGetHandler(httpd_req_t *req)
{
	httpd_resp_set_status(req, "307 Temporary Redirect");
	httpd_resp_set_hdr(req, "Location", "/");
	httpd_resp_send(req, "<html><body>Wrong</body></html>", 0); // Response body can be empty

	return ESP_OK;
}

esp_err_t BrewEngine::apiPostHandler(httpd_req_t *req)
{
	string stringBuffer;
	char buf[256];
	uint32_t ret;
	uint32_t remaining = req->content_len;

	while (remaining > 0)
	{
		// Read the data
		int nBytes = (std::min<uint32_t>)(remaining, sizeof(buf));

		if ((ret = httpd_req_recv(req, buf, nBytes)) <= 0)
		{
			if (ret == HTTPD_SOCK_ERR_TIMEOUT)
			{
				// Timeout, just continue
				continue;
			}

			return ESP_FAIL;
		}

		size_t bytes_read = ret;

		remaining -= bytes_read;

		// append to buffer
		stringBuffer.append((char *)buf, bytes_read);
	}

	string commandResult = mainInstance->processCommand(stringBuffer);

	const char *returnBuf = commandResult.c_str();
	httpd_resp_set_type(req, "text/plain");
	httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
	httpd_resp_sendstr(req, returnBuf);

	return ESP_OK;
}

// needed for cors
esp_err_t BrewEngine::apiOptionsHandler(httpd_req_t *req)
{
	string commandResult = "";
	const char *returnBuf = commandResult.c_str();

	httpd_resp_set_type(req, "text/plain");
	httpd_resp_set_hdr(req, "Access-Control-Max-Age", "1728000");
	httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, PATCH, OPTIONS");
	httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Authorization,Content-Type,Accept,Origin,User-Agent,DNT,Cache-Control,X-Mx-ReqToken,Keep-Alive,X-Requested-With,If-Modified-Since");
	httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
	httpd_resp_set_status(req, "204");
	httpd_resp_sendstr(req, returnBuf);

	return ESP_OK;
}

string BrewEngine::to_iso_8601(std::chrono::time_point<std::chrono::system_clock> t)
{

	// convert to time_t which will represent the number of
	// seconds since the UNIX epoch, UTC 00:00:00 Thursday, 1st. January 1970
	auto epoch_seconds = std::chrono::system_clock::to_time_t(t);

	// Format this as date time to seconds resolution
	// e.g. 2016-08-30T08:18:51
	std::stringstream stream;
	stream << std::put_time(gmtime(&epoch_seconds), "%FT%T");

	// If we now convert back to a time_point we will get the time truncated
	// to whole seconds
	auto truncated = std::chrono::system_clock::from_time_t(epoch_seconds);

	// Now we subtract this seconds count from the original time to
	// get the number of extra microseconds..
	auto delta_us = std::chrono::duration_cast<std::chrono::microseconds>(t - truncated).count();

	// And append this to the output stream as fractional seconds
	// e.g. 2016-08-30T08:18:51.867479
	stream << "." << std::fixed << std::setw(6) << std::setfill('0') << delta_us;

	return stream.str();
}