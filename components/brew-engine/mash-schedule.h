#ifndef _MashSchedule_H_
#define _MashSchedule_H_

#include <deque>
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

protected:
private:
};

class MashSchedule
{
public:
    string name;
    std::deque<MashStep *> steps;

    json to_json()
    {
        this->sort_steps();

        json jSchedule;
        jSchedule["name"] = this->name;
        json jSteps = json::array({});

        for (auto const &step : this->steps)
        {
            json jStep;
            jStep["index"] = step->index;
            jStep["name"] = step->name;
            jStep["temperature"] = step->temperature;
            jStep["stepTime"] = step->stepTime;
            jStep["time"] = step->time;
            jStep["extendStepTimeIfNeeded"] = step->extendStepTimeIfNeeded;
            jSteps.push_back(jStep);
        }

        jSchedule["steps"] = jSteps;

        return jSchedule;
    };

    void from_json(json jsonData)
    {
        this->name = jsonData["name"];
        json steps = jsonData["steps"];

        for (auto &el : steps.items())
        {
            auto jStep = el.value();

            auto step = new MashStep();
            step->index = jStep["index"];
            step->name = jStep["name"];
            step->temperature = jStep["temperature"];
            step->stepTime = jStep["stepTime"];
            step->time = jStep["time"];
            step->extendStepTimeIfNeeded = jStep["extendStepTimeIfNeeded"];
            this->steps.push_back(step);
        }
    };

    void sort_steps()
    {
        // sort our steps by index
        sort(this->steps.begin(), this->steps.end(), [](MashStep *s1, MashStep *s2)
             { return (s1->index < s2->index); });
    }

protected:
private:
};

#endif
