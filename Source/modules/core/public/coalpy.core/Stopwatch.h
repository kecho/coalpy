#pragma once
#include <chrono>

namespace coalpy
{

class Stopwatch
{
public:
    void start();
    unsigned timeMicroSeconds() const;
    unsigned long long timeMicroSecondsLong() const;

private:
    using TimeType = std::chrono::time_point<std::chrono::high_resolution_clock>;
    TimeType m_timeStart;
};

}
