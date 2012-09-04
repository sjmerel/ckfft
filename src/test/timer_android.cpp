#include "timer.h"


void Timer::init()
{
    clock_gettime(CLOCK_MONOTONIC, &s_startTime);

    // ticks are nanoseconds
    s_msPerTick = 0.000001f;
}

timespec Timer::s_startTime;

