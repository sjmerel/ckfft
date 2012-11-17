#include "stats.h"
#include "ckfft/debug.h"
#include <assert.h>
#include <algorithm>

Stats::Stats(int reserve) 
{
    m_values.reserve(reserve);
}

void Stats::sample(float value)
{
    std::vector<float>::iterator it = std::upper_bound(m_values.begin(), m_values.end(), value);
    m_values.insert(it, value);
}

void Stats::reset()
{
    m_values.clear();
}

int Stats::getCount() const 
{ 
    return (int) m_values.size();
}

float Stats::getMax() const 
{
    assert(!m_values.empty());
    return m_values.back();
}

float Stats::getMin() const 
{
    assert(!m_values.empty());
    return m_values.front();
}

float Stats::getSum() const 
{
    float sum = 0.0f;
    for (int i = 0; i < m_values.size(); ++i)
    {
        sum += m_values[i];
    }
    return sum;
}

float Stats::getMean() const 
{
    assert(!m_values.empty());
    return getSum()/getCount();
}

float Stats::getMedian() const
{
    int n = (int) m_values.size();
    if (n % 2 == 0)
    {
        // mean of 2 middle values
        return (m_values[n/2] + m_values[n/2-1]) * 0.5f;
    }
    else
    {
        return m_values[n/2];
    }
}

void Stats::print()
{
//    for (int i = 0; i < m_values.size(); ++i)
//    {
//        CKFFT_PRINTF("%f\n", m_values[i]);
//    }
    float min = getMin();
    float max = getMax();
    int numBins = 20;
    float binSize = (max-min)/numBins;
    float binMax = min + binSize;
    int binCount = 0;
    for (int i = 0; i < m_values.size(); ++i)
    {
        float value = m_values[i];
        if (value < binMax)
        {
//        CKFFT_PRINTF("%f (%f)\n", value, binMax);
            ++binCount;
        }
        if (value >= binMax)
        {
            CKFFT_PRINTF("%f: %d\n", binMax, binCount);
            binCount = 0;
            binMax += binSize;
            --i;
        }
    }
    CKFFT_PRINTF("%f: %d\n", binMax, binCount);
    CKFFT_PRINTF("mean: %f\n", getMean());
    CKFFT_PRINTF("median: %f\n", getMedian());

}
