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
    bool show;
    bool useForControl;
    bool connected;
    float compensateAbsolute;
    float compensateRelative;
    float lastTemp;
    ds18b20_device_handle_t handle;

    json to_json()
    {
        json jSensor;
        jSensor["id"] = to_string(this->id); // js doesn't support uint64_t, so we convert to string
        jSensor["name"] = this->name;
        jSensor["color"] = this->color;
        jSensor["show"] = this->show;
        jSensor["useForControl"] = this->useForControl;
        jSensor["connected"] = this->connected;
        jSensor["compensateAbsolute"] = this->compensateAbsolute;
        jSensor["compensateRelative"] = this->compensateRelative;
        jSensor["lastTemp"] = (double)((int)(this->lastTemp * 10)) / 10; // round float to 0.1 for display

        return jSensor;
    };

    void from_json(const json &jsonData)
    {
        string stringId = jsonData["id"].get<string>(); // js doesn't support uint64_t, so we convert it from string
        uint64_t sensorId = std::stoull(stringId);

        this->id = sensorId;
        this->name = (string)jsonData["name"];
        this->color = (string)jsonData["color"];

        if (!jsonData["show"].is_null() && jsonData["show"].is_boolean())
        {
            this->show = jsonData["show"];
        }
        else
        {
            this->show = true;
        }

        if (!jsonData["useForControl"].is_null() && jsonData["useForControl"].is_boolean())
        {
            this->useForControl = jsonData["useForControl"];
        }
        else
        {
            this->useForControl = true;
        }

        if (!jsonData["compensateAbsolute"].is_null() && jsonData["compensateAbsolute"].is_number_float())
        {
            this->compensateAbsolute = (float)jsonData["compensateAbsolute"];
        }
        else
        {
            this->compensateAbsolute = 0;
        }

        if (!jsonData["compensateRelative"].is_null() && jsonData["compensateRelative"].is_number_float())
        {
            this->compensateRelative = (float)jsonData["compensateRelative"];
        }
        else
        {
            this->compensateRelative = 1;
        }

        // will be set by detection
        this->connected = false;
    };

protected:
private:
};

#endif /* _TemperatureSensor_H_ */