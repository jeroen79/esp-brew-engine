#ifndef _MashStep_H_
#define _MashStep_H_

#include "nlohmann_json.hpp"
using namespace std;
using json = nlohmann::json;

class MashStep
{
public:
    uint index;
    string name;
    int temperature;
    int stepTime;
    int time;
    bool extendStepTimeIfNeeded; // if true, we extend the step time untit we reach our temperatue

    json to_json()
    {

        json jStep;
        jStep["index"] = this->index;
        jStep["name"] = this->name;
        jStep["temperature"] = this->temperature;
        jStep["stepTime"] = this->stepTime;
        jStep["time"] = this->time;
        jStep["extendStepTimeIfNeeded"] = this->extendStepTimeIfNeeded;

        return jStep;
    }

    void from_json(const json &jsonData)
    {
        this->index = jsonData["index"].get<int>();
        this->name = jsonData["name"].get<string>();
        this->temperature = jsonData["temperature"].get<int>();
        this->stepTime = jsonData["stepTime"].get<int>();
        this->time = jsonData["time"].get<int>();
        this->extendStepTimeIfNeeded = jsonData["extendStepTimeIfNeeded"].get<bool>();
    }

protected:
private:
};

#endif
