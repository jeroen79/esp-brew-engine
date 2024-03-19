#ifndef _Heater_H_
#define _Heater_H_

#include "nlohmann_json.hpp"

using namespace std;
using json = nlohmann::json;

class Heater
{
public:
    uint8_t id;
    string name;
    uint8_t preference;
    gpio_num_t pinNr;
    uint16_t watt;
    bool useForMash;
    bool useForBoil;
    uint8_t burnTime; // runtime burn Time flag, doesn't go to json, in %
    bool burn;        // runtime burn flag true means burn now
    bool enabled;     // runtime flag to make it easyer to filter in loops, is set based on mode and mash/boil

    json to_json()
    {
        json jHeater;
        jHeater["id"] = this->id;
        jHeater["name"] = this->name;
        jHeater["preference"] = this->preference;
        jHeater["pinNr"] = this->pinNr;
        jHeater["watt"] = this->watt;
        jHeater["useForMash"] = this->useForMash;
        jHeater["useForBoil"] = this->useForBoil;

        return jHeater;
    };

    void from_json(const json &jsonData)
    {
        this->id = jsonData["id"].get<uint>();
        this->name = jsonData["name"].get<string>();
        this->preference = jsonData["preference"].get<uint>();
        this->pinNr = (gpio_num_t)jsonData["pinNr"].get<uint>();

        if (!jsonData["watt"].is_null() && jsonData["watt"].is_number())
        {
            this->watt = (uint16_t)jsonData["watt"];
        }
        else
        {
            this->watt = 0;
        }

        if (!jsonData["useForMash"].is_null() && jsonData["useForMash"].is_boolean())
        {
            this->useForMash = jsonData["useForMash"];
        }
        else
        {
            this->useForMash = true;
        }

        if (!jsonData["useForBoil"].is_null() && jsonData["useForBoil"].is_boolean())
        {
            this->useForBoil = jsonData["useForBoil"];
        }
        else
        {
            this->useForBoil = true;
        }

        this->burnTime = 0;
        this->burn = false;
        this->enabled = false;
    };

protected:
private:
};

#endif /* _Heater_H_ */