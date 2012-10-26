#include "ckfft/ckfft.h"
#include "ckfft/fft.h"
#include "ckfft/debug.h"
#include "ckfft/context.h"
#include "ckfft/math.h"

using namespace ckfft;

////////////////////////////////////////
// TODO:
//  - NEON
//  - fixed point?
//  - alignment?
//  - optimizations for real inputs?
//  - is DIF better than DIT for NEON optimization?
//  - stride for when complex values aren't in their own array?
//  - optimized way to interleave non-interleaved complex data?
//  - optimized way to scale data?
//  - is non-interleaved data faster? it might simplify API.
//  - Windows build
//  - SIMD optimizations for Windows, Mac?
//  - expTable should only be n-1 elements, not n
//  - notes about memory requirements, scaling
//  - for real fft, pack last value into first's imaginary part?

////////////////////////////////////////

extern "C"
{

CkFftContext* CkFftInit(int count, int inverse, void* userBuf, size_t* userBufSize) 
{
    if (count <= 0)
    {
        return NULL;
    }
    if (!isPowerOfTwo(count))
    {
        return NULL;
    }
    if (userBuf && !userBufSize)
    {
        return NULL;
    }

    return (CkFftContext*) CkFftContextBase::create(count, inverse, false, userBuf, userBufSize);
}

int CkFft(CkFftContext* context, const CkFftComplex* input, CkFftComplex* output)
{
    if (!context || context->real || !input || !output || input == output)
    {
        return 0;
    }
    else
    {
        fft(context, input, output, context->count);
        return 1;
    }
}

void CkFftShutdown(CkFftContext* context)  
{
    CkFftContext::destroy(context);
}

}
