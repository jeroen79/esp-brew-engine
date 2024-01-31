/*
 * esp-brew-engine
 * Copyright (C) Dekien Jeroen 2024
 *
 */
#ifndef INCLUDE_PIDCONTROLLER_HPP_
#define INCLUDE_PIDCONTROLLER_HPP_

#include <stdexcept>
#include <iostream>
using namespace std;
using std::cout;

class PIDController
{

private:
    double previousError;
    double integral;
    double previousActual;

    double kp; // Proportional
    double ki; // Integral
    double kd; // Derivative
    double max;
    double min = 0;

    bool firstRun = true;

    void addToIntegral(double i)
    {
        double newIntegral = integral + i;

        integral = clamp(newIntegral, min, max);

        if (debug)
        {
            cout << "new integral:" + to_string(integral) + "\n";
        }
    }

public:
    bool debug = false;

    PIDController(double p, double i, double d)
    {
        if (p == 0 || i == 0 || d == 0)
        {
            throw std::invalid_argument("K,I or P should not be empty!");
        }

        this->kp = p;
        this->ki = i;
        this->kd = d;

        previousError = 0.0;
        integral = 0.0;
    }

    void setMax(double max)
    {
        this->max = max;
    }

    void setMin(double min)
    {
        this->min = min;
    }

    double getOutput(double actual, double setpoint)
    {
        previousActual = actual;

        // Error
        double error = setpoint - actual;

        // Proportional
        double p = kp * error;

        double i = 0;
        double d = 0;

        // skip i and d on first run
        if (!this->firstRun)
        {
            if (ki > 0)
            {
                // Integral 10
                addToIntegral(error);

                i = ki * (integral / 2); // Trapezoidal integration
                i = clamp(i, min, max);
            }

            // Derivative
            d = kd * (error - previousError);
        }
        previousError = error;

        double output = p + i + d;

        if (debug)
        {
            cout << "p:" + to_string(p) + " i:" + to_string(i) + " d:" + to_string(d) + " output:" + to_string(output) + "\n";
        }

        output = clamp(output, min, max);

        this->firstRun = false;

        return output;
    }
};

#endif // INCLUDE_PIDCONTROLLER_HPP_