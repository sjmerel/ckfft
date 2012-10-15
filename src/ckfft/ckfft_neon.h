#pragma once
#include "ckfft/ckfft.h"

void fftimpl_neon(CkFftContext* context, const CkFftComplex* input, CkFftComplex* output, int count, int stride);

