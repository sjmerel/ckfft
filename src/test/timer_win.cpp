#include "timer.h"
#include "ckfft/platform.h"


void Timer::init()
{
    ckfft::uint64 ticksPerSec;
    QueryPerformanceFrequency((LARGE_INTEGER*) &ticksPerSec);
    s_msPerTick = 1000.0f / ticksPerSec;
}



