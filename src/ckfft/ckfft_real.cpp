#include "ckfft/ckfft_real.h"
#include "ckfft/math.h"
#include "ckfft/context.h"
#include "ckfft/fft.h"

using namespace ckfft;

// see http://www.engineeringproductivitytools.com/stuff/T0001/PT10.HTM

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
        int countDiv2 = count/2;

        fft(context, (const CkFftComplex*) input, output, countDiv2, 2);

        output[countDiv2] = output[0];

        const CkFftComplex* exp0 = context->expTable;
        const CkFftComplex* exp1 = context->expTable + countDiv2;
        int m = count/4 + 1;
        for (int i = 0; i < m; ++i)
        {
            // XXX note that this actually calculates output[count/4] twice
            CkFftComplex z0 = output[i];
            CkFftComplex z1 = output[countDiv2 - i];

            CkFftComplex sum;
            CkFftComplex diff;
            CkFftComplex f;
            CkFftComplex c;

            sum.real = z0.real + z1.real;
            sum.imag = z0.imag - z1.imag;
            diff.real = z0.real - z1.real;
            diff.imag = z0.imag + z1.imag;
            f.real = -(exp0->imag);
            f.imag = exp0->real;
            multiply(f, diff, c);
            subtract(sum, c, output[i]);

            diff.real = -diff.real;
            sum.imag = -sum.imag;
            f.real = -(exp1->imag);
            f.imag = exp1->real;
            multiply(f, diff, c);
            subtract(sum, c, output[countDiv2 - i]);

            ++exp0;
            --exp1;
        }

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
        int countDiv2 = count/2;

        CkFftComplex* inputBuf = context->inputBuf;

        const CkFftComplex* exp0 = context->expTable;
        const CkFftComplex* exp1 = context->expTable + countDiv2;
        int m = count/4 + 1;
        for (int i = 0; i < m; ++i)
        {
            // XXX note that this actually calculates output[count/4] twice
            CkFftComplex z0 = input[i];
            CkFftComplex z1 = input[countDiv2 - i];

            CkFftComplex sum;
            CkFftComplex diff;
            CkFftComplex f;
            CkFftComplex c;

            sum.real = z0.real + z1.real;
            sum.imag = z0.imag - z1.imag;
            diff.real = z0.real - z1.real;
            diff.imag = z0.imag + z1.imag;
            f.real = -(exp0->imag);
            f.imag = exp0->real;
            multiply(f, diff, c);
            add(sum, c, inputBuf[i]);

            diff.real = -diff.real;
            sum.imag = -sum.imag;
            f.real = -(exp1->imag);
            f.imag = exp1->real;
            multiply(f, diff, c);
            add(sum, c, inputBuf[countDiv2 - i]);

            ++exp0;
            --exp1;
        }

        fft(context, inputBuf, (CkFftComplex*) output, countDiv2, 2);

        return 1;
    }
}

void CkFftRealShutdown(CkFftRealContext* context)  
{
    CkFftContextBase::destroy(context);
}


} // extern "C"
