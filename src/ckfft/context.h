#pragma once
#include "ckfft/ckfft.h"

struct CkFftContextBase
{
    bool neon;
    bool inverse;
    bool real;
    int count;
    const CkFftComplex* expTable;
    CkFftComplex* inputBuf;
    bool ownBuf; // true if memory was allocated by us, rather than user

    static CkFftContextBase* create(int count, bool inverse, bool real, void* buf, size_t* bufSize);
    static void destroy(CkFftContextBase*);

private:
    CkFftContextBase();
};

struct _CkFftContext : public CkFftContextBase {};

struct _CkFftRealContext : public CkFftContextBase {};

