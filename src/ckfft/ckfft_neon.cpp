#include "ckfft/ckfft.h"
#include "ckfft/platform.h"
#include "ckfft/debug.h"
#include "ckfft/context.h"

#if CKFFT_ARM_NEON
#include <arm_neon.h>

namespace
{

#if 0
inline void vmul(const CkFftComplex* x, const CkFftComplex* y, CkFftComplex* out)
{
    // (a + bi)(c + di) = (ac - bd) + (bc + ad)i
    // vld1: a b , c d
    // vmul: a b , c d , ac bd
    // vrev: b a , c d , ac bd
    // vmul: b a , bc ad , ac bd
    // vneg: b a , bc ad , ac -bd
    // vtrn: b a , -bd ad , ac bc 
    // vadd: ac-bd bc+ad , -bd ad , ac bc 

    asm volatile
        (
         "vld1.32 d0, [%[x]]\n\t"   // a b
         "vld1.32 d1, [%[y]]\n\t"   // a b         | c d
         "vmul.f32 d2, d0, d1\n\t"  // a b         | c d    | ac bd
         "vrev64.32 d0, d0\n\t"     // b a         | c d    | ac bd
         "vmul.f32 d1, d0, d1\n\t"  // b a         | bc ad  | ac bd
         "vneg.f32 s5, s5\n\t"      // b a         | bc ad  | ac -bd
         "vtrn.32 d2, d1\n\t"       // b a         | -bd ad | ac bc
         "vadd.f32 d0, d1, d2\n\t"  // ac-bd bc+ad | -bd ad | ac bc
         "vst1.32 d0, [%[out]]\n\t"

         : [out] "+r" (out)
         : [x] "r" (x), [y] "r" (y)
         : "d0", "d1", "d2", "cc" // ??
        );
}

inline void vmulq(const CkFftComplex* x0, const CkFftComplex* y0, CkFftComplex* out0,
                  const CkFftComplex* x1, const CkFftComplex* y1, CkFftComplex* out1)
{
    asm volatile
        (
         "vld1.32 d0, [%[x0]]\n\t"   // a0 b0
         "vld1.32 d1, [%[x1]]\n\t"   // a0 b0 | a1 b1
         "vld1.32 d2, [%[y0]]\n\t"   // a0 b0 | a1 b1 | c0 d0
         "vld1.32 d3, [%[y1]]\n\t"   // a0 b0 | a1 b1 | c0 d0     | c1 d1
         "vmul.f32 q2, q0, q1\n\t"   // a0 b0 | a1 b1 | c0 d0     | c1 d1     | a0c0 b0d0 | a1c1 b1d1
         "vrev64.32 q0, q0\n\t"      // b0 a0 | b1 a1 | c0 d0     | c1 d1     | a0c0 b0d0 | a1c1 b1d1
         "vmul.f32 q1, q0, q1\n\t"   // b0 a0 | b1 a1 | b0c0 a0d0 | b1c1 a1d1 | a0c0 b0d0 | a1c1 b1d1

         "vneg.f32 s5, s5\n\t"      // b a         | bc ad  | ac -bd
         "vtrn.32 d2, d1\n\t"       // b a         | -bd ad | ac bc
         "vadd.f32 d0, d1, d2\n\t"  // ac-bd bc+ad | -bd ad | ac bc
         "vst1.32 d0, [%[out]]\n\t"

         : [out] "+r" (out)
         : [x] "r" (x), [y] "r" (y)
         : "d0", "d1", "d2", "cc" // ??
        );
}

#endif

inline void vmul(const float32x4x2_t& x, const float32x4x2_t& y, float32x4x2_t& out)
{
    // (a + bi)(c + di) = (ac - bd) + (bc + ad)i
    float32x4_t ac = vmulq_f32(x.val[0], y.val[0]);
    float32x4_t bd = vmulq_f32(x.val[1], y.val[1]);
    float32x4_t bc = vmulq_f32(x.val[1], y.val[0]);
    float32x4_t ad = vmulq_f32(x.val[0], y.val[1]);
    out.val[0] = vsubq_f32(ac, bd);
    out.val[1] = vaddq_f32(bc, ad);
}

inline void vmul(const float32x2x2_t& x, const float32x2x2_t& y, float32x2x2_t& out)
{
    // (a + bi)(c + di) = (ac - bd) + (bc + ad)i
    float32x2_t ac = vmul_f32(x.val[0], y.val[0]);
    float32x2_t bd = vmul_f32(x.val[1], y.val[1]);
    float32x2_t bc = vmul_f32(x.val[1], y.val[0]);
    float32x2_t ad = vmul_f32(x.val[0], y.val[1]);
    out.val[0] = vsub_f32(ac, bd);
    out.val[1] = vadd_f32(bc, ad);
}

inline void vadd(const float32x4x2_t& x, const float32x4x2_t& y, float32x4x2_t& out)
{
    out.val[0] = vaddq_f32(x.val[0], y.val[0]);
    out.val[1] = vaddq_f32(x.val[1], y.val[1]);
}

inline void vadd(const float32x2x2_t& x, const float32x2x2_t& y, float32x2x2_t& out)
{
    out.val[0] = vadd_f32(x.val[0], y.val[0]);
    out.val[1] = vadd_f32(x.val[1], y.val[1]);
}

inline void vsub(const float32x4x2_t& x, const float32x4x2_t& y, float32x4x2_t& out)
{
    out.val[0] = vsubq_f32(x.val[0], y.val[0]);
    out.val[1] = vsubq_f32(x.val[1], y.val[1]);
}

inline void vsub(const float32x2x2_t& x, const float32x2x2_t& y, float32x2x2_t& out)
{
    out.val[0] = vsub_f32(x.val[0], y.val[0]);
    out.val[1] = vsub_f32(x.val[1], y.val[1]);
}

inline void add(const CkFftComplex& x, const CkFftComplex& y, CkFftComplex& out)
{
#if 0
    register float* px = (float*) &x;
    register float* py = (float*) &y;
    register float* pout = (float*) &out;
    asm volatile
        (
         "vld1.32 d0, [%[px]]\n\t"
         "vld1.32 d1, [%[py]]\n\t"
         "vadd.f32 d0, d0, d1\n\t"
         "vst1.32 d0, [%[pout]]\n\t"

         : [pout] "=r" (pout)
         : [px] "r" (px), [py] "r" (py)
         : "d0", "d1", "cc", "memory"// ??
        );
#else
    out.real = x.real + y.real;
    out.imag = x.imag + y.imag;
#endif
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

////////////////////////////////////////

void fftimpl_neon(CkFftContext* context, const CkFftComplex* input, CkFftComplex* output, int count, int stride)
{
    /*
    if (count == 2)
    {
#if 0
        register int inc = stride * sizeof(CkFftComplex);
        asm volatile
            (
             "vld1.32 d0, [%[input]], %[inc]\n\t" // d0 = *input
             "vld1.32 d1, [%[input]]\n\t"         // d1 = *(input + stride)
             "vsub.f32 d2, d0, d1\n\t"            // d2 = d0 - d1
             "vadd.f32 d1, d0, d1\n\t"            // d1 = d0 + d1
             "vst1.32 {d1-d2}, [%[output]]\n\t"   // *output = d1, *(output+1) = d2

             :
             : [input] "r" (input), [inc] "r" (inc), [output] "r" (output)
             : "d0", "d1", "d2", "memory"
            );
#else
        float32x2_t input0_v;
        input0_v = vld1_f32((const float*) input);
        float32x2_t inputs_v;
        inputs_v = vld1_f32((const float*) (input + stride));

        float32x2_t output0_v;
        output0_v = vadd_f32(input0_v, inputs_v);
        float32x2_t output1_v;
        output1_v = vsub_f32(input0_v, inputs_v);

        float32x4_t output_v = vcombine_f32(output0_v, output1_v);
        vst1q_f32((float*) output, output_v);
#endif
    }
    */

    if (count == 4)
    {
        // TODO vectorize?

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
    else if (count == 8)
    {
        int stride2 = stride * 2;
        int stride3 = stride * 3;
        int stride4 = stride * 4;

        const CkFftComplex* in0 = input;
        CkFftComplex* out = output;
        CkFftComplex* outEnd = out + 8;
        while (out < outEnd)
        {
            const CkFftComplex* in1 = in0 + stride4;
            add(*in0, *in1, out[0]);
            subtract(*in0, *in1, out[1]);

            in0 += stride;
            out += 2;
        }

        CkFftComplex* exp = context->expTable;

        float32x2x2_t f1w_v, f2w2_v, f3w3_v;
        float32x2x2_t sum02_v, diff02_v, sum13_v, diff13_v;

        CkFftComplex* out0 = output;
        CkFftComplex* out1 = out0 + 2;
        CkFftComplex* out2 = out1 + 2;
        CkFftComplex* out3 = out2 + 2;

        float32x2x2_t out0_v = vld2_f32((const float*) out0);
        float32x2x2_t out1_v = vld2_f32((const float*) out1);
        float32x2x2_t out2_v = vld2_f32((const float*) out2);
        float32x2x2_t out3_v = vld2_f32((const float*) out3);

        float32x2x2_t exp1_v;
        exp1_v = vld2_lane_f32((const float*) exp, exp1_v, 0);
        exp1_v = vld2_lane_f32((const float*) (exp + stride), exp1_v, 1);

        float32x2x2_t exp2_v;
        exp2_v = vld2_lane_f32((const float*) exp, exp2_v, 0);
        exp2_v = vld2_lane_f32((const float*) (exp + stride2), exp2_v, 1);

        float32x2x2_t exp3_v;
        exp3_v = vld2_lane_f32((const float*) exp, exp3_v, 0);
        exp3_v = vld2_lane_f32((const float*) (exp + stride3), exp3_v, 1);

        vmul(out1_v, exp1_v, f1w_v);
        vmul(out2_v, exp2_v, f2w2_v);
        vmul(out3_v, exp3_v, f3w3_v);

        vadd(out0_v, f2w2_v, sum02_v);
        vsub(out0_v, f2w2_v, diff02_v);
        vadd(f1w_v, f3w3_v, sum13_v);
        vsub(f1w_v, f3w3_v, diff13_v);

        vadd(sum02_v, sum13_v, out0_v);
        vsub(sum02_v, sum13_v, out2_v);

        // TODO optimize this?
        if (context->inverse)
        {
            out1_v.val[0] = vsub_f32(diff02_v.val[0], diff13_v.val[1]);
            out1_v.val[1] = vadd_f32(diff02_v.val[1], diff13_v.val[0]);
            out3_v.val[0] = vadd_f32(diff02_v.val[0], diff13_v.val[1]);
            out3_v.val[1] = vsub_f32(diff02_v.val[1], diff13_v.val[0]);
        }
        else
        {
            out1_v.val[0] = vadd_f32(diff02_v.val[0], diff13_v.val[1]);
            out1_v.val[1] = vsub_f32(diff02_v.val[1], diff13_v.val[0]);
            out3_v.val[0] = vsub_f32(diff02_v.val[0], diff13_v.val[1]);
            out3_v.val[1] = vadd_f32(diff02_v.val[1], diff13_v.val[0]);
        }

        vst2_f32((float*) out0, out0_v);
        vst2_f32((float*) out1, out1_v);
        vst2_f32((float*) out2, out2_v);
        vst2_f32((float*) out3, out3_v);
    }
    else
    {
        assert((count & 0x3) == 0);

        int n = count / 4;
        int stride2 = stride * 2;
        int stride3 = stride * 3;
        int stride4 = stride * 4;

        const CkFftComplex* in = input;
        CkFftComplex* out = output;
        CkFftComplex* outEnd = out + count;
        while (out < outEnd)
        {
            fftimpl_neon(context, in, out, n, stride4);
            in += stride;
            out += n;
        }

        CkFftComplex* exp1 = context->expTable;
        CkFftComplex* exp2 = exp1;
        CkFftComplex* exp3 = exp1;

        float32x4x2_t f1w_v, f2w2_v, f3w3_v;
        float32x4x2_t sum02_v, diff02_v, sum13_v, diff13_v;

        CkFftComplex* out0 = output;
        CkFftComplex* out1 = out0 + n;
        CkFftComplex* out2 = out1 + n;
        CkFftComplex* out3 = out2 + n;

        int m = n/4;
        for (int i = 0; i < m; ++i)
        {
            float32x4x2_t out0_v = vld2q_f32((const float*) out0);
            float32x4x2_t out1_v = vld2q_f32((const float*) out1);
            float32x4x2_t out2_v = vld2q_f32((const float*) out2);
            float32x4x2_t out3_v = vld2q_f32((const float*) out3);

            float32x4x2_t exp1_v;
            exp1_v = vld2q_lane_f32((const float*) exp1, exp1_v, 0);
            exp1 += stride;
            exp1_v = vld2q_lane_f32((const float*) exp1, exp1_v, 1);
            exp1 += stride;
            exp1_v = vld2q_lane_f32((const float*) exp1, exp1_v, 2);
            exp1 += stride;
            exp1_v = vld2q_lane_f32((const float*) exp1, exp1_v, 3);
            exp1 += stride;

            float32x4x2_t exp2_v;
            exp2_v = vld2q_lane_f32((const float*) exp2, exp2_v, 0);
            exp2 += stride2;
            exp2_v = vld2q_lane_f32((const float*) exp2, exp2_v, 1);
            exp2 += stride2;
            exp2_v = vld2q_lane_f32((const float*) exp2, exp2_v, 2);
            exp2 += stride2;
            exp2_v = vld2q_lane_f32((const float*) exp2, exp2_v, 3);
            exp2 += stride2;

            float32x4x2_t exp3_v;
            exp3_v = vld2q_lane_f32((const float*) exp3, exp3_v, 0);
            exp3 += stride3;
            exp3_v = vld2q_lane_f32((const float*) exp3, exp3_v, 1);
            exp3 += stride3;
            exp3_v = vld2q_lane_f32((const float*) exp3, exp3_v, 2);
            exp3 += stride3;
            exp3_v = vld2q_lane_f32((const float*) exp3, exp3_v, 3);
            exp3 += stride3;

            // TODO use vmla, vmls?
            // alignment?

            vmul(out1_v, exp1_v, f1w_v);
            vmul(out2_v, exp2_v, f2w2_v);
            vmul(out3_v, exp3_v, f3w3_v);

            vadd(out0_v, f2w2_v, sum02_v);
            vsub(out0_v, f2w2_v, diff02_v);
            vadd(f1w_v, f3w3_v, sum13_v);
            vsub(f1w_v, f3w3_v, diff13_v);

            vadd(sum02_v, sum13_v, out0_v);
            vsub(sum02_v, sum13_v, out2_v);

            // TODO optimize this?
            if (context->inverse)
            {
                out1_v.val[0] = vsubq_f32(diff02_v.val[0], diff13_v.val[1]);
                out1_v.val[1] = vaddq_f32(diff02_v.val[1], diff13_v.val[0]);
                out3_v.val[0] = vaddq_f32(diff02_v.val[0], diff13_v.val[1]);
                out3_v.val[1] = vsubq_f32(diff02_v.val[1], diff13_v.val[0]);
            }
            else
            {
                out1_v.val[0] = vaddq_f32(diff02_v.val[0], diff13_v.val[1]);
                out1_v.val[1] = vsubq_f32(diff02_v.val[1], diff13_v.val[0]);
                out3_v.val[0] = vsubq_f32(diff02_v.val[0], diff13_v.val[1]);
                out3_v.val[1] = vaddq_f32(diff02_v.val[1], diff13_v.val[0]);
            }

            vst2q_f32((float*) out0, out0_v);
            vst2q_f32((float*) out1, out1_v);
            vst2q_f32((float*) out2, out2_v);
            vst2q_f32((float*) out3, out3_v);

            out0 += 4;
            out1 += 4;
            out2 += 4;
            out3 += 4;
        }
    }
}
#else
void fftimpl_neon(CkFftContext* context, const CkFftComplex* input, CkFftComplex* output, int count, int stride) {}
#endif


