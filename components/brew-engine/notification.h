#ifndef _Notification_H_
#define _Notification_H_

#include <chrono>
#include "nlohmann_json.hpp"

using namespace std;
using namespace std::chrono;
using json = nlohmann::json;

class Notification
{
public:
    string name;
    int timeFromStart;
    system_clock::time_point timePoint;
    bool buzzer;

    json to_json()
    {
        int seconds = 0;
        if (this->timePoint.time_since_epoch() != decltype(this->timePoint)::duration::zero())
        {
            seconds = std::chrono::duration_cast<std::chrono::seconds>(this->timePoint.time_since_epoch()).count();
        }

        json jNotification;
        jNotification["name"] = this->name;
        jNotification["timeFromStart"] = this->timeFromStart;
        jNotification["timePoint"] = seconds;
        jNotification["buzzer"] = this->buzzer;

        return jNotification;
    };

protected:
private:
};

#endif
