#ifndef _TemperatureSensor_H_
#define _TemperatureSensor_H_

#include "nlohmann_json.hpp"

using namespace std;
using json = nlohmann::json;

class TemperatureSensor
{
public:
    uint64_t id;
    string name;
    string color;
    bool enabled;
    bool connected;
    float offset;
    float lastTemp;
    ds18b20_device_handle_t handle;

    json to_json()
    {
        json jSensor;
        jSensor["id"] = to_string(this->id); // js doesn't support uint64_t, so we convert to string
        jSensor["name"] = this->name;
        jSensor["color"] = this->color;
        jSensor["enabled"] = this->enabled;
        jSensor["connected"] = this->connected;
        jSensor["offset"] = this->offset;
        jSensor["lastTemp"] = (double)((int)(this->lastTemp * 10)) / 10; // round float to 0.1 for display

        return jSensor;
    };

    void from_json(json jsonData)
    {
        string stringId = jsonData["id"].get<string>(); // js doesn't support uint64_t, so we convert it from string
        uint64_t sensorId = std::stoull(stringId);

        this->id = sensorId;
        this->name = (string)jsonData["name"];
        this->color = (string)jsonData["color"];

        if (!jsonData["enabled"].is_null() && jsonData["enabled"].is_boolean())
        {
            this->enabled = jsonData["enabled"];
        }
        else
        {
            this->enabled = true;
        }

        if (!jsonData["offset"].is_null() && jsonData["offset"].is_number_float())
        {
            this->offset = (float)jsonData["offset"];
        }
        else
        {
            this->offset = 0;
        }
        // will be set by detection
        this->connected = false;
    };

protected:
private:
};

#endif /* _TemperatureSensor_H_ */