#include "ckfft/context.h"
#include "ckfft/platform.h"
#include "ckfft/debug.h"
#include <math.h>

#if CKFFT_PLATFORM_ANDROID
#  include <cpu-features.h>
#endif

_CkFftContext::_CkFftContext(int _count, bool _inverse) :
    neon(false),
    inverse(_inverse),
    count(_count),
    expTable(NULL)
{
#if CKFFT_PLATFORM_ANDROID
    // on Android, need to check for NEON support at runtime
    neon = ((android_getCpuFeatures() & ANDROID_CPU_ARM_FEATURE_NEON) == ANDROID_CPU_ARM_FEATURE_NEON);
#elif CKFFT_PLATFORM_IOS
#  if CKFFT_ARM_NEON
    // on iOS, all armv7(s) devices support NEON
    neon = true;
#  endif
#endif

    // TODO
    // - memory allocation
    expTable = new CkFftComplex[count];
    for (int i = 0; i < count; ++i)
    {
        float theta = -2.0f * M_PI * i / count;
        expTable[i].real = cosf(theta);
        expTable[i].imag = sinf(theta);
        if (inverse)
        {
            expTable[i].imag = -expTable[i].imag;
        }
    }
}

_CkFftContext::~_CkFftContext()
{
    delete[] expTable;
    expTable = NULL;
}

