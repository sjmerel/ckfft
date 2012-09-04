#include "timer.h"

void Timer::init()
{
    mach_timebase_info_data_t info;
    mach_timebase_info(&info);
    s_msPerTick = 1e-6f * (float) info.numer / (float) info.denom;
}


