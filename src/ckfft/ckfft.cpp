#include "ckfft/ckfft.h"
#include "ckfft/debug.h"
#include <assert.h>
#include <math.h>

#if CKFFT_PLATFORM_ANDROID
#  include <cpu-features.h>
#endif

////////////////////////////////////////
// OPTIMIZATIONS:
//  - factor out count/2, etc
//  - Complex operations as members? As plain functions?
//  - NEON
//  - more special cases than count==1
//  - precalculate expf factors?  
//  - lookup table for expf factors? (save space with lookup table for cos only?)
//  - fixed point?
//  - optimizations for real inputs?
//  - memory allocations

struct _CkFftContext
{
    _CkFftContext(int count);
    ~_CkFftContext();

    bool neon;
    int count;
    CkFftComplex* expTable;
};

_CkFftContext::_CkFftContext(int _count) :
    neon(false),
    count(_count),
    expTable(NULL)
{
#if CKFFT_PLATFORM_ANDROID
    // on Android, need to check for NEON support at runtime
    neon = ((android_getCpuFeatures() & ANDROID_CPU_ARM_FEATURE_NEON) == ANDROID_CPU_ARM_FEATURE_NEON);
#elif CKFFT_PLATFORM_IOS
    // on iOS, all armv7(s) devices support NEON
#  if CKFFT_ARM_NEON
    neon = true;
#  endif
#endif

    expTable = new CkFftComplex[count]; // TODO memory allocation
    // store cosines & sines separately?  only need count/2?

    for (int i = 0; i < count; ++i)
    {
        float theta = -2.0f * M_PI * i / count;
        expTable[i].real = cosf(theta);
        expTable[i].imag = sinf(theta);
    }
}

_CkFftContext::~_CkFftContext()
{
    delete[] expTable;
    expTable = NULL;
}

////////////////////////////////////////

namespace
{

inline void add(const CkFftComplex& a, const CkFftComplex& b, CkFftComplex& out)
{
    out.real = a.real + b.real;
    out.imag = a.imag + b.imag;
}

inline void subtract(const CkFftComplex& a, const CkFftComplex& b, CkFftComplex& out)
{
    out.real = a.real - b.real;
    out.imag = a.imag - b.imag;
}

inline void multiply(const CkFftComplex& a, const CkFftComplex& b, CkFftComplex& out)
{
    out.real = a.real * b.real - a.imag * b.imag;
    out.imag = a.imag * b.real + b.imag * a.real;
}

inline void expi(float a, CkFftComplex& out)
{
    out.real = cosf(a);
    out.imag = sinf(a);
}

void fftimpl(CkFftContext* context, const CkFftComplex* input, CkFftComplex* output, int count, int stride, bool inverse)
{
    if (count == 1)
    {
        *output = *input;
    }
    else
    {
        // DFT of even and odd elements
        fftimpl(context, input, output, count/2, stride*2, inverse);
        fftimpl(context, input + stride, output + count/2, count/2, stride*2, inverse);

        // reshuffle
        CkFftComplex tmp;
        int n = count / 2;
        int expIndex = 0;
        for (int i = 0; i < n; ++i)
        {
            tmp = output[i];

            // output[i]           = tmp + exp(-2*pi*I*i/count) * output[i + count/2];
            // output[i + count/2] = tmp - exp(-2*pi*I*i/count) * output[i + count/2];

            assert(expIndex < context->count);
            CkFftComplex a = context->expTable[expIndex];
            if (inverse)
            {
                a.imag = -a.imag;
            }
            expIndex += stride;

//            CKFFT_PRINTF("%f/%f + i %f/%f\n", a.real, x.real, a.imag, x.imag);

            CkFftComplex b;
            multiply(a, output[i + n], b);

            add(tmp, b, output[i]);
            subtract(tmp, b, output[i + n]);
        }
    }
}

bool isPowerOfTwo(unsigned int x)
{
    return ((x != 0) && !(x & (x - 1)));
}

}

////////////////////////////////////////

extern "C"
{

CkFftContext* CkFftInit(int count) 
{
    // XXX better error handling?
    assert(isPowerOfTwo(count)); 
    assert(count > 0);

    // TODO memory allocation
    CkFftContext* context = new CkFftContext(count);
    return context;
}

void CkFft(CkFftContext* context, const CkFftComplex* input, CkFftComplex* output, int count)
{
    // XXX better error handling?
    assert(isPowerOfTwo(count)); 
    assert(count > 0);
    assert(input != output);

    fftimpl(context, input, output, count, 1, false);
}

void CkInvFft(CkFftContext* context, const CkFftComplex* input, CkFftComplex* output, int count)
{
    // XXX better error handling?
    assert(isPowerOfTwo(count)); 
    assert(count > 0);
    assert(input != output);

    fftimpl(context, input, output, count, 1, true);
}

void CkFftShutdown(CkFftContext* context)  
{
    delete context;
}

}
