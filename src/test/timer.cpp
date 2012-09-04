#include "timer.h"


Timer::Timer() :
    m_startTick(0),
    m_elapsedTicks(0)
{}

void Timer::reset()
{
    m_elapsedTicks = 0;
    if (isRunning())
    {
        m_startTick = getTick();
    }
}

float Timer::getElapsedMs() const
{
    uint64 elapsed = m_elapsedTicks;
    if (isRunning())
    {
        elapsed += getTick() - m_startTick;
    }
    return s_msPerTick * elapsed;
}

////////////////////////////////////////

float Timer::s_msPerTick = 0.0f;


