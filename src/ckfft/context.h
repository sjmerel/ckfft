#pragma once
#include "ckfft/ckfft.h"
#include <assert.h>

struct _CkFftContext
{
    _CkFftContext(int count, bool inverse);
    ~_CkFftContext();

    bool neon;
    bool inverse;
    int count;
    CkFftComplex* expTable;
};
