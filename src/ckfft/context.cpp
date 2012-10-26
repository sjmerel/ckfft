#include "ckfft/context.h"
#include "ckfft/platform.h"
#include "ckfft/debug.h"
#include <math.h>
#include <new>

#if CKFFT_PLATFORM_ANDROID
#  include <cpu-features.h>
#endif

CkFftContextBase::CkFftContextBase() :
    neon(false),
    inverse(false),
    real(false),
    count(0),
    expTable(NULL),
    inputBuf(NULL),
    ownBuf(false)
{}

CkFftContextBase* CkFftContextBase::create(int count, bool inverse, bool real, void* userBuf, size_t* userBufSize)
{
    // size of context object
    int contextSize = sizeof(CkFftContextBase);
    if (contextSize % sizeof(CkFftComplex))
    {
        // alignment XXX
        contextSize += sizeof(CkFftComplex) - (contextSize % sizeof(CkFftComplex));
    }

    // size of lookup table
    int expTableSize = count * sizeof(CkFftComplex);

    // size of temp input buf (for inverse real FFT only)
    int inputBufSize = 0;
    if (real && inverse)
    {
        inputBufSize = (count/2 + 1) * sizeof(CkFftComplex);
    }

    int reqBufSize = contextSize + expTableSize + inputBufSize;

    if (userBufSize && (!userBuf || *userBufSize < reqBufSize))
    {
        *userBufSize = reqBufSize;
        return NULL;
    }

    // allocate buffer if needed
    void* buf = NULL;
    if (userBuf)
    {
        buf = userBuf;
    }
    else
    {
        // allocate buffer
        buf = malloc(reqBufSize);
        if (!buf)
        {
            return NULL;
        }
    }

    // initialize
    CkFftContextBase* context = new (buf) CkFftContextBase();

    context->neon = false;
#if CKFFT_PLATFORM_ANDROID
    // on Android, need to check for NEON support at runtime
    context->neon = ((android_getCpuFeatures() & ANDROID_CPU_ARM_FEATURE_NEON) == ANDROID_CPU_ARM_FEATURE_NEON);
#elif CKFFT_PLATFORM_IOS
#  if CKFFT_ARM_NEON
    // on iOS, all armv7(s) devices support NEON
    context->neon = true;
#  endif
#endif

    // lookup table
    CkFftComplex* expBuf = (CkFftComplex*) ((char*) buf + contextSize);
    for (int i = 0; i < count; ++i)
    {
        float theta = -2.0f * M_PI * i / count;
        expBuf[i].real = cosf(theta);
        expBuf[i].imag = sinf(theta);
        if (inverse)
        {
            expBuf[i].imag = -expBuf[i].imag;
        }
    }

    context->inverse = inverse;
    context->real = real;
    context->count = count;
    context->expTable = expBuf;
    if (real && inverse)
    {
        context->inputBuf = expBuf + count;
    }
    context->ownBuf = (userBuf == NULL);

    return context;
}

void CkFftContextBase::destroy(CkFftContextBase* context)
{
    if (context && context->ownBuf)
    {
        free(context);
    }
}

////////////////////////////////////////
