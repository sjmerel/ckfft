#pragma once

#include "ckfft/platform.h"

#if CKFFT_PLATFORM_ANDROID
#  include <time.h>
#elif CKFFT_PLATFORM_IOS || CKFFT_PLATFORM_MACOS
#  include <mach/mach.h>
#  include <mach/mach_time.h>
#elif CKFFT_PLATFORM_WIN
#  include <windows.h>
#endif


class Timer
{
public:
    static void init();

    Timer();
    
    void start()
    {
        if (!isRunning())
        {
            m_startTick = getTick();
        }
    }

    void stop()
    {
        if (isRunning())
        {
            m_elapsedTicks += getTick() - m_startTick;
            m_startTick = 0;
        }
    }

    void reset();

    float getElapsedMs() const;

    bool isRunning() const { return m_startTick != 0; }

private:
    uint64 m_startTick;
    uint64 m_elapsedTicks;
    static float s_msPerTick;
#if CKFFT_PLATFORM_ANDROID
    static timespec s_startTime;
#endif

    uint64 getTick() const
    {
#if CKFFT_PLATFORM_ANDROID
        timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        time_t s = (now.tv_sec - s_startTime.tv_sec);
        time_t ns = (now.tv_nsec - s_startTime.tv_nsec);
        return s * 1000000000LL + ns;
#elif CKFFT_PLATFORM_IOS || CKFFT_PLATFORM_MACOS
        return mach_absolute_time();
#elif CKFFT_PLATFORM_WIN
        uint64 now;
        QueryPerformanceCounter((LARGE_INTEGER*) &now);
        return now;
#endif
    }
};



