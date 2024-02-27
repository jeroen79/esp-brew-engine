#ifndef _Notification_H_
#define _Notification_H_

#include <chrono>

using namespace std;
using namespace std::chrono;

class Notification
{
public:
    string name;
    int timeFromStart;
    system_clock::time_point timePoint;
    bool buzzer;

protected:
private:
};

#endif
