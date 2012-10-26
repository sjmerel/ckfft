#pragma once

#include "ckfft/ckfft.h"

namespace ckfft
{
    inline bool isPowerOfTwo(unsigned int x)
    {
        return ((x != 0) && !(x & (x - 1)));
    }

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
        out.imag = a.imag * b.real + a.real * b.imag;
    }

}
