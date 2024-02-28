#ifndef _MashSchedule_H_
#define _MashSchedule_H_

#include <deque>
#include "nlohmann_json.hpp"
#include "mash-step.h"
#include "notification.h"

using namespace std;
using json = nlohmann::json;

class MashSchedule
{
public:
    string name;
    bool boil; // if true boil else mash
    std::deque<MashStep *> steps;
    std::deque<Notification *> notifications;

    json to_json()
    {
        this->sort_steps();
        this->sort_notifications();

        json jSchedule;
        jSchedule["name"] = this->name;
        jSchedule["boil"] = this->boil;
        json jSteps = json::array({});

        for (auto const &step : this->steps)
        {
            json jStep = step->to_json();
            jSteps.push_back(jStep);
        }

        jSchedule["steps"] = jSteps;

        json jNotifications = json::array({});
        for (auto const &notification : this->notifications)
        {
            json jNotification = notification->to_json();
            jNotifications.push_back(jNotification);
        }

        jSchedule["notifications"] = jNotifications;

        return jSchedule;
    };

    void from_json(json jsonData)
    {
        this->name = jsonData["name"];

        if (!jsonData["boil"].is_null() && jsonData["boil"].is_boolean())
        {
            this->boil = jsonData["boil"].get<bool>();
        }
        else
        {
            this->boil = false;
        }

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

        json notifications = jsonData["notifications"];

        if (notifications.is_array())
        {
            for (auto &el : notifications.items())
            {
                auto jNotification = el.value();

                auto notification = new Notification();
                notification->name = jNotification["name"];
                notification->timeFromStart = jNotification["timeFromStart"];
                // notification->timePoint = {};
                notification->buzzer = jNotification["buzzer"];
                this->notifications.push_back(notification);
            }
        }
    };

    void sort_steps()
    {
        // sort our steps by index
        sort(this->steps.begin(), this->steps.end(), [](MashStep *s1, MashStep *s2)
             { return (s1->index < s2->index); });
    }

    void sort_notifications()
    {
        // sort our notifications by time
        sort(this->notifications.begin(), this->notifications.end(), [](Notification *n1, Notification *n2)
             { return (n1->timeFromStart < n2->timeFromStart); });
    }

protected:
private:
};

#endif
