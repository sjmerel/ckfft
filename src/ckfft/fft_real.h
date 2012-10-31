#pragma once
#include "ckfft/ckfft.h"


namespace ckfft
{

void fft_real(
        CkFftContext* context, 
        const float* input, 
        CkFftComplex* output, 
        int count);

void fft_real_inverse(
        CkFftContext* context, 
        const CkFftComplex* input, 
        float* output, 
        int count,
        void* tmpBuf,
        size_t* tmpBufSize);

}


