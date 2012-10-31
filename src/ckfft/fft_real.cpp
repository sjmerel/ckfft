#include "ckfft/fft.h"
#include "ckfft/fft_real_default.h"
#include "ckfft/fft_real_neon.h"
#include "ckfft/math.h"
#include "ckfft/context.h"


namespace ckfft
{

void fft_real(CkFftContext* context, 
         const float* input, 
         CkFftComplex* output, 
         int count)
{
    // handle trivial cases here, so we don't have to check for them in fft_real_default
    if (count == 1)
    {
        output->real = *input;
        output->imag = 0.0f;
    }
    else if (count == 2)
    {
        // radix-2 loop unrolled
        output[0].real = input[0] + input[1];
        output[0].imag = 0.0f;
        output[1].real = input[0] - input[1];
        output[1].imag = 0.0f;
    }
    else
    {
#if 1
        // NEON enabled
        if (context->neon)
        {
            fft_real_neon(context, input, output, count);
        }
        else
        {
            fft_real_default(context, input, output, count);
        }
#else
        fft_real_default(context, input, output, count);
#endif
    }
}

void fft_real_inverse(CkFftContext* context, 
         const CkFftComplex* input, 
         float* output, 
         int count,
         void* tmpBuf,
         size_t* tmpBufSize)
{
    if (tmpBuf == NULL)
    {
        *tmpBufSize = count * sizeof(CkFftComplex);
    }
    else
    {
        // handle trivial cases here, so we don't have to check for them in fft_real_default
        if (count == 1)
        {
            *output = input->real;
        }
        else if (count == 2)
        {
            // radix-2 loop unrolled
            output[0] = input[0].real + input[1].real;
            output[1] = input[0].real - input[1].real;
        }
        else
        {
#if 1
            // NEON enabled
            if (context->neon)
            {
                fft_real_inverse_neon(context, input, output, count, (CkFftComplex*) tmpBuf);
            }
            else
            {
                fft_real_inverse_default(context, input, output, count, (CkFftComplex*) tmpBuf);
            }
#else
            fft_real_inverse_default(context, input, output, count, (CkFftComplex*) tmpBuf);
#endif
        }
    }
}

} // namespace ckfft

