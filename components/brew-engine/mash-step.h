#ifndef _MashStep_H_
#define _MashStep_H_

using namespace std;

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

#endif
