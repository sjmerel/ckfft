#include "ckfft/ckfft_real.h"
#include "ckfft/math.h"
#include "ckfft/context.h"
#include "ckfft/fft.h"

using namespace ckfft;

namespace
{

// see http://www.engineeringproductivitytools.com/stuff/T0001/PT10.HTM

void separate(const CkFftComplex& exp, const CkFftComplex& a, const CkFftComplex& b, CkFftComplex& out)
{
    CkFftComplex bConj;
    bConj.real = b.real;
    bConj.imag = -b.imag;

    CkFftComplex sum;
    add(a, bConj, sum);
    CkFftComplex diff;
    subtract(a, bConj, diff);

    CkFftComplex f;
    f.real = -exp.imag;
    f.imag = exp.real;

    CkFftComplex c;
    multiply(f, diff, c);
    subtract(sum, c, out);
}

void separateInv(const CkFftComplex& exp, const CkFftComplex& a, const CkFftComplex& b, CkFftComplex& out)
{
    CkFftComplex bConj;
    bConj.real = b.real;
    bConj.imag = -b.imag;

    CkFftComplex sum;
    add(a, bConj, sum);
    CkFftComplex diff;
    subtract(a, bConj, diff);

    CkFftComplex f;
    f.real = -exp.imag;
    f.imag = exp.real;

    CkFftComplex c;
    multiply(f, diff, c);
    add(sum, c, out);
}

} // namespace

////////////////////////////////////////

extern "C"
{

CkFftRealContext* CkFftRealInit(int count, int inverse, void* userBuf, size_t* userBufSize) 
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

    return (CkFftRealContext*) CkFftContextBase::create(count, inverse, true, userBuf, userBufSize);
}

int CkFftReal(CkFftRealContext* context, const float* input, CkFftComplex* output)
{
    if (!context || !context->real || context->inverse || !input || !output)
    {
        return 0;
    }
    else
    {
        int count = context->count;
        int halfCount = count/2;
        fft(context, (const CkFftComplex*) input, output, halfCount, 2);

        CkFftComplex z = output[0];
        output[halfCount] = z;

        const CkFftComplex* exp0 = context->expTable;
        const CkFftComplex* exp1 = context->expTable + halfCount;
        int quarterCount = halfCount/2;
        for (int i = 0; i < quarterCount+1; ++i)
        {
            // XXX note that this actually calculates output[quarterCount] twice
            CkFftComplex z0 = output[i];
            CkFftComplex z1 = output[halfCount - i];
            separate(*exp0, z0, z1, output[i]);
            separate(*exp1, z1, z0, output[halfCount - i]);
            ++exp0;
            --exp1;
        }

        /*
        // center term
        CkFftComplex center;
        center.real = z.real - z.imag;
        center.imag = 0.0f;
        output[halfCount] = center;
        */

        return 1;
    }

}

int CkFftRealInverse(CkFftRealContext* context, const CkFftComplex* input, float* output)
{
    if (!context || !context->real || !context->inverse || !input || !output)
    {
        return 0;
    }
    else
    {
        int count = context->count;
        int halfCount = count/2;

        CkFftComplex* inputBuf = context->inputBuf;

        const CkFftComplex* exp0 = context->expTable;
        const CkFftComplex* exp1 = context->expTable + halfCount;
        int quarterCount = halfCount/2;
        for (int i = 0; i < quarterCount+1; ++i)
        {
            // XXX note that this actually calculates output[quarterCount] twice
            separateInv(*exp0, input[i], input[halfCount-i], inputBuf[i]);
            separateInv(*exp1, input[halfCount-i], input[i], inputBuf[halfCount - i]);
            ++exp0;
            --exp1;
        }

        /*
        // center term
        CkFftComplex center;
        center.real = input[0].real - input[0].imag;
        center.imag = 0.0f;
        tmpInput[halfCount] = center;
        */

        fft(context, inputBuf, (CkFftComplex*) output, halfCount, 2);

        return 1;
    }
}

void CkFftRealShutdown(CkFftRealContext* context)  
{
    CkFftContextBase::destroy(context);
}


} // extern "C"
