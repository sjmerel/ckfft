#pragma once
#include "ckfft/ckfft.h"

struct CkFftContextBase;


namespace ckfft
{

void fft(CkFftContextBase* context, const CkFftComplex* input, CkFftComplex* output, int count, int expTableDiv = 1);

}

