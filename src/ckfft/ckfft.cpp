#include "ckfft/ckfft.h"
#include "ckfft/ckfft_neon.h"
#include "ckfft/debug.h"
#include "ckfft/context.h"
#include <assert.h>

////////////////////////////////////////
// OPTIMIZATIONS:
//  - NEON
//  - more special cases than count==1
//  - larger radices
//  - fixed point?
//  - optimizations for real inputs?
//  - is DIF better than DIT for NEON optimization?
//  - memory allocations


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
    out.imag = a.imag * b.real + a.real * b.imag;
}

// see http://www.cmlab.csie.ntu.edu.tw/cml/dsp/training/coding/transform/fft.html
void fftimpl(CkFftContext* context, const CkFftComplex* input, CkFftComplex* output, int count, int stride)
{
    if (count == 2)
    {
        // radix-2 loop unrolled

        // this code will be called at the deepest recursion level for FFT sizes
        // that are not a power of 4.

        //output[0] = input[0] + input[stride];
        //output[1] = input[0] - input[stride];
        const CkFftComplex* in0 = input;
        const CkFftComplex* in1 = input + stride;
        add(*in0, *in1, output[0]);
        subtract(*in0, *in1, output[1]);
    }
    else if (count == 4)
    {
        // radix-4 loop unrolled

        // this code will be called at the deepest recursion level for FFT sizes
        // that are a power of 4.

        const CkFftComplex* in = input;
        CkFftComplex* out = output;
        CkFftComplex* outEnd = out + 4;
        while (out < outEnd)
        {
            *out = *in;
            in += stride;
            ++out;
        }

        CkFftComplex sum02, diff02, sum13, diff13;

        CkFftComplex* out0 = output;
        CkFftComplex* out1 = out0 + 1;
        CkFftComplex* out2 = out1 + 1;
        CkFftComplex* out3 = out2 + 1;

        add(*out0, *out2, sum02);
        subtract(*out0, *out2, diff02);
        add(*out1, *out3, sum13);
        subtract(*out1, *out3, diff13);

        add(sum02, sum13, *out0);
        subtract(sum02, sum13, *out2);
        if (context->inverse)
        {
            out1->real = diff02.real - diff13.imag;
            out1->imag = diff02.imag + diff13.real;
            out3->real = diff02.real + diff13.imag;
            out3->imag = diff02.imag - diff13.real;
        }
        else
        {
            out1->real = diff02.real + diff13.imag;
            out1->imag = diff02.imag - diff13.real;
            out3->real = diff02.real - diff13.imag;
            out3->imag = diff02.imag + diff13.real;
        }
    }
    else
    {
        // radix-4
        assert((count & 0x3) == 0);

        int n = count / 4;
        int s4 = stride * 4;

        // calculate FFT of each 1/4
        const CkFftComplex* in = input;
        CkFftComplex* out = output;
        CkFftComplex* outEnd = out + n*4;
        while (out < outEnd)
        {
            fftimpl(context, in, out, n, s4);
            in += stride;
            out += n;
        }

        CkFftComplex* exp1 = context->expTable;
        CkFftComplex* exp2 = exp1;
        CkFftComplex* exp3 = exp1;

        CkFftComplex f1w, f2w2, f3w3;
        CkFftComplex sum02, diff02, sum13, diff13;

        CkFftComplex* out0 = output;
        CkFftComplex* out1 = out0 + n;
        CkFftComplex* out2 = out1 + n;
        CkFftComplex* out3 = out2 + n;

        for (int i = 0; i < n; ++i)
        {
            // NOTE: use vmla and vmls when vectorizing!
            /*
               W = exp(-2*pi*I/N)

               X0 = F0 +   F1*W + F2*W2 +   F3*W3
               X1 = F0 - I*F1*W - F2*W2 + I*F3*W3
               X2 = F0 -   F1*W + F2*W2 -   F3*W3
               X3 = F0 + I*F1*W - F2*W2 - I*F3*W3

               X0 = (F0 + F2*W2) +   (F1*W + F3*W3) = sum02 + sum13
               X1 = (F0 - F2*W2) - I*(F1*W - F3*W3) = diff02 - I*diff13
               X2 = (F0 + F2*W2) -   (F1*W + F3*W3) = sum02 - sum13
               X3 = (F0 - F2*W2) + I*(F1*W - F3*W3) = diff02 + I*diff13

             */

            // f1w = F1*W
            // f2w2 = F2*W2
            // f3w3 = F3*W3
            multiply(*out1, *exp1, f1w);
            multiply(*out2, *exp2, f2w2);
            multiply(*out3, *exp3, f3w3);

            // sum02  = F0 + f2w2
            // diff02 = F0 - f2w2
            // sum13  = f1w + f3w3
            // diff13 = f1w - f3w3
            add(*out0, f2w2, sum02);
            subtract(*out0, f2w2, diff02);
            add(f1w, f3w3, sum13);
            subtract(f1w, f3w3, diff13);

            // x + I*y = (x.real + I*x.imag) + I*(y.real + I*y.imag)
            //         = x.real + I*x.imag + I*y.real - y.imag
            //         = (x.real - y.imag) + I*(x.imag + y.real)
            // x - I*y = (x.real + I*x.imag) - I*(y.real + I*y.imag)
            //         = x.real + I*x.imag - I*y.real + y.imag
            //         = (x.real + y.imag) + I*(x.imag - y.real)
            add(sum02, sum13, *out0);
            subtract(sum02, sum13, *out2);
            if (context->inverse)
            {
                out1->real = diff02.real - diff13.imag;
                out1->imag = diff02.imag + diff13.real;
                out3->real = diff02.real + diff13.imag;
                out3->imag = diff02.imag - diff13.real;
            }
            else
            {
                out1->real = diff02.real + diff13.imag;
                out1->imag = diff02.imag - diff13.real;
                out3->real = diff02.real - diff13.imag;
                out3->imag = diff02.imag + diff13.real;
            }

            exp1 += stride;
            exp2 += 2*stride;
            exp3 += 3*stride;

            ++out0;
            ++out1;
            ++out2;
            ++out3;
        }
        /*
        else
        {
            // radix-2
            // this is never called; radix-2 is only used for count==2 above

            int n = count / 2;
            int s2 = stride * 2;

            // DFT of even and odd elements
            fftimpl(context, input, output, n, s2);
            fftimpl(context, input + stride, output + n, n, s2);

            // combine
            CkFftComplex* out0 = output;
            CkFftComplex* out1 = output + n;
            CkFftComplex* out0End = output + n;
            CkFftComplex* exp = context->expTable;
            CkFftComplex tmp, b;
            while (out0 < out0End)
            {
                // tmp = output[i];
                // output[i]           = tmp + exp(-2*pi*I*i/count) * output[i + count/2];
                // output[i + count/2] = tmp - exp(-2*pi*I*i/count) * output[i + count/2];

                multiply(*exp, *out1, b);

                tmp = *out0;
                add(tmp, b, *out0);
                subtract(tmp, b, *out1);

                ++out0;
                ++out1;
                exp += stride;
            }
        }
        */
    }
}

bool isPowerOfTwo(unsigned int x)
{
    return ((x != 0) && !(x & (x - 1)));
}

void fft(CkFftContext* context, const CkFftComplex* input, CkFftComplex* output, int count)
{
    // XXX better error handling?
    assert(isPowerOfTwo(count)); 
    assert(count > 0);
    assert(input != output);

    // handle trivial case here, so we don't have to check for it in fftimpl
    if (count == 1)
    {
        *output = *input;
    }
    else
    {
#if 0
        if (context->neon)
        {
            fftimpl_neon(context, input, output, count, 1);
        }
        else
        {
            fftimpl(context, input, output, count, 1);
        }
#else
        fftimpl(context, input, output, count, 1);
#endif
    }
}

}

////////////////////////////////////////

extern "C"
{

CkFftContext* CkFftInit(int count, int inverse) 
{
    // XXX better error handling?
    assert(isPowerOfTwo(count)); 
    assert(count > 0);

    // TODO memory allocation
    CkFftContext* context = new CkFftContext(count, (inverse != 0));
    return context;
}

void CkFft(CkFftContext* context, const CkFftComplex* input, CkFftComplex* output, int count)
{
    // TODO do we need count here?
    fft(context, input, output, count);
}

void CkFftShutdown(CkFftContext* context)  
{
    delete context;
}

}
