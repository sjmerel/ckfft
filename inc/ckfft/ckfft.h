#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

void CkFftInit();

struct CkFftComplex
{
    float real;
    float imag;
};

void CkFft(const CkFftComplex* input, CkFftComplex* output, int count);
void CkFft(const float* input, float* output, int count);

void CkFftShutdown();

#ifdef __cplusplus
} // extern "C"
#endif
