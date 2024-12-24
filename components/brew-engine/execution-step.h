#ifndef _ExecutionStep_H_
#define _ExecutionStep_H_

#include <chrono>
#include "nlohmann_json.hpp"

using namespace std;
using namespace std::chrono;
using json = nlohmann::json;

class ExecutionStep
{
public:
    system_clock::time_point time;
    float temperature;
    bool extendIfNeeded;
    bool allowBoost;

    json to_json()
    {
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(this->time.time_since_epoch()).count();

        json jStep;
        jStep["temperature"] = this->temperature;
        jStep["time"] = seconds;
        jStep["extendIfNeeded"] = this->extendIfNeeded;
        jStep["allowBoost"] = this->allowBoost;

        return jStep;
    };

protected:
private:
};

#endif /* _ExecutionStep_H_ */