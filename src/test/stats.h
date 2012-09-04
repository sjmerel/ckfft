#pragma once

#include "ckfft/platform.h"
#include <assert.h>

class Stats
{
public:
    Stats();

    void sample(float);

    void reset();

    int getCount() const;

    float getMax() const;
    float getMin() const;
    float getSum() const;
    float getMean() const;

private:
    float m_sum;
    float m_max;
    float m_min;
    int m_count;
};


////////////////////////////////////////

inline
int Stats::getCount() const 
{ 
    return m_count; 
}

inline
float Stats::getMax() const 
{
    assert(m_count > 0); 
    return m_max; 
}

inline
float Stats::getMin() const 
{
    assert(m_count > 0); 
    return m_min; 
}

inline
float Stats::getSum() const 
{
    return m_sum; 
}

inline
float Stats::getMean() const 
{
    assert(m_count > 0); 
    return m_sum/m_count; 
}

