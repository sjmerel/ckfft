#include "ckfft/ckfft.h"
#include <assert.h>
#include <math.h>

////////////////////////////////////////
// OPTIMIZATIONS:
//  - factor out count/2, etc
//  - Complex operations as members? As plain functions?
//  - NEON
//  - more special cases than count==1
//  - precalculate expf factors?  
//  - lookup table for expf factors?
//  - fixed point?
//  - optimizations for real inputs?

namespace
{

void add(const CkFftComplex& a, const CkFftComplex& b, CkFftComplex& out)
{
    out.real = a.real + b.real;
    out.imag = a.imag + b.imag;
}

void subtract(const CkFftComplex& a, const CkFftComplex& b, CkFftComplex& out)
{
    out.real = a.real - b.real;
    out.imag = a.imag - b.imag;
}

void multiply(const CkFftComplex& a, const CkFftComplex& b, CkFftComplex& out)
{
    out.real = a.real * b.real - a.imag * b.imag;
    out.imag = a.imag * b.real + b.imag * a.real;
}

void expi(float a, CkFftComplex& out)
{
    out.real = cosf(a);
    out.imag = sinf(a);
}

void fftimpl(const CkFftComplex* input, CkFftComplex* output, int count, int stride, bool inverse)
{
    if (count == 1)
    {
        *output = *input;
    }
    else
    {
        // DFT of even and odd elements
        fftimpl(input, output, count/2, stride*2, inverse);
        fftimpl(input + stride, output + count/2, count/2, stride*2, inverse);

        // reshuffle
        CkFftComplex tmp;
        for (int i = 0; i < count/2; ++i)
        {
            tmp = output[i];

            // output[i]           = tmp + exp(-2*pi*I*i/count) * output[i + count/2];
            // output[i + count/2] = tmp - exp(-2*pi*I*i/count) * output[i + count/2];

            CkFftComplex a;
            float theta = -2.0f * M_PI * i / count; // TODO factor out of loop
            if (inverse)
            {
                theta = -theta; // TODO pass theta in as argument?
            }
            expi(theta, a);

            CkFftComplex b;
            multiply(a, output[i + count/2], b);

            add(tmp, b, output[i]);
            subtract(tmp, b, output[i + count/2]);
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

void CkFftInit() {}

void CkFft(const CkFftComplex* input, CkFftComplex* output, int count)
{
    // XXX better error handling?
    assert(isPowerOfTwo(count)); 
    assert(count > 0);
    assert(input != output);

    fftimpl(input, output, count, 1, false);
}

void CkInvFft(const CkFftComplex* input, CkFftComplex* output, int count)
{
    // XXX better error handling?
    assert(isPowerOfTwo(count)); 
    assert(count > 0);
    assert(input != output);

    fftimpl(input, output, count, 1, true);

    // TODO do this in fftimpl?
    for (int i = 0; i < count; ++i)
    {
        output[i].real /= count;
        output[i].imag /= count;
    }
}

void CkFftShutdown() {}

}
