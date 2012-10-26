#pragma once
#include <stdlib.h>
#include "ckfft/complex.h"

#ifdef __cplusplus
extern "C"
{
#endif



typedef struct _CkFftRealContext CkFftRealContext;


CkFftRealContext* CkFftRealInit(int count, int inverse, void* buf, size_t* bufSize);

// TODO option to return only count/2 + 1 elements?
int CkFftReal(CkFftRealContext* context, const float* input, CkFftComplex* output);

int CkFftRealInverse(CkFftRealContext* context, const CkFftComplex* input, float* output);

void CkFftRealShutdown(CkFftRealContext*);






#ifdef __cplusplus
} // extern "C"
#endif

