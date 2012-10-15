#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

struct _CkFftComplex
{
    float real;
    float imag;
};
typedef struct _CkFftComplex CkFftComplex;

typedef struct _CkFftContext CkFftContext;

// NOTE: no scaling is applied to the results of either the forward or inverse FFT, so if you apply both to a data set, the result will be scaled by count.

CkFftContext* CkFftInit(int count, int inverse);
void CkFft(CkFftContext*, const CkFftComplex* input, CkFftComplex* output);
void CkFftShutdown(CkFftContext*);

#ifdef __cplusplus
} // extern "C"
#endif
