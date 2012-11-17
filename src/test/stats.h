#pragma once

#include "ckfft/platform.h"
#include <vector>

class Stats
{
public:
    Stats(int reserve);

    void sample(float);

    void reset();

    int getCount() const;

    float getMax() const;
    float getMin() const;
    float getSum() const;
    float getMean() const;
    float getMedian() const;

    void print();

private:
    std::vector<float> m_values;
};


