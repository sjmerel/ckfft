#pragma once
#include "ckfft/ckfft.h"

struct CkFftContextBase;


namespace ckfft
{

void fft_neon(CkFftContextBase* context, const CkFftComplex* input, CkFftComplex* output, int count, int stride, int expTableDiv = 1);

}


