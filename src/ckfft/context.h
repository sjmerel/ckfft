#pragma once
#include "ckfft/ckfft.h"
#include <assert.h>

struct _CkFftContext
{
    _CkFftContext(int count, bool inverse, void* expTableBuf, bool ownBuf);

    bool neon;
    bool inverse;
    int count;
    CkFftComplex* expTable;
    bool ownBuf; // true if memory was allocated by us, rather than user
};
