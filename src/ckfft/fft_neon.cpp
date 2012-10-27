#include "ckfft/platform.h"
#include "ckfft/debug.h"
#include "ckfft/context.h"
#include "ckfft/math.h"
#include <assert.h>

#if CKFFT_ARM_NEON
#include <arm_neon.h>

using namespace ckfft;

namespace
{

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

} // namespace

////////////////////////////////////////

namespace ckfft
{

void fft_neon(CkFftContextBase* context, const CkFftComplex* input, CkFftComplex* output, int count, int stride, int expTableDiv)
{
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
        const CkFftComplex* in0 = input;
        CkFftComplex* out = output;
        CkFftComplex* outEnd = out + 8;
        int stride4 = stride * 4;
        while (out < outEnd)
        {
            const CkFftComplex* in1 = in0 + stride4;
            add(*in0, *in1, out[0]);
            subtract(*in0, *in1, out[1]);

            in0 += stride;
            out += 2;
        }

        int expTableStride = stride * expTableDiv;
        const CkFftComplex* exp = context->expTable;

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
        exp1_v = vld2_lane_f32((const float*) (exp + expTableStride), exp1_v, 1);

        float32x2x2_t exp2_v;
        exp2_v = vld2_lane_f32((const float*) exp, exp2_v, 0);
        exp2_v = vld2_lane_f32((const float*) (exp + expTableStride*2), exp2_v, 1);

        float32x2x2_t exp3_v;
        exp3_v = vld2_lane_f32((const float*) exp, exp3_v, 0);
        exp3_v = vld2_lane_f32((const float*) (exp + expTableStride*3), exp3_v, 1);

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

        const CkFftComplex* in = input;
        CkFftComplex* out = output;
        CkFftComplex* outEnd = out + count;
        int stride4 = stride * 4;
        while (out < outEnd)
        {
            fft_neon(context, in, out, n, stride4, expTableDiv);
            in += stride;
            out += n;
        }

        const CkFftComplex* exp1 = context->expTable;
        const CkFftComplex* exp2 = exp1;
        const CkFftComplex* exp3 = exp1;
        int expTableStride1 = stride * expTableDiv;
        int expTableStride2 = expTableStride1 * 2;
        int expTableStride3 = expTableStride1 * 3;

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
            exp1 += expTableStride1;
            exp1_v = vld2q_lane_f32((const float*) exp1, exp1_v, 1);
            exp1 += expTableStride1;
            exp1_v = vld2q_lane_f32((const float*) exp1, exp1_v, 2);
            exp1 += expTableStride1;
            exp1_v = vld2q_lane_f32((const float*) exp1, exp1_v, 3);
            exp1 += expTableStride1;

            float32x4x2_t exp2_v;
            exp2_v = vld2q_lane_f32((const float*) exp2, exp2_v, 0);
            exp2 += expTableStride2;
            exp2_v = vld2q_lane_f32((const float*) exp2, exp2_v, 1);
            exp2 += expTableStride2;
            exp2_v = vld2q_lane_f32((const float*) exp2, exp2_v, 2);
            exp2 += expTableStride2;
            exp2_v = vld2q_lane_f32((const float*) exp2, exp2_v, 3);
            exp2 += expTableStride2;

            float32x4x2_t exp3_v;
            exp3_v = vld2q_lane_f32((const float*) exp3, exp3_v, 0);
            exp3 += expTableStride3;
            exp3_v = vld2q_lane_f32((const float*) exp3, exp3_v, 1);
            exp3 += expTableStride3;
            exp3_v = vld2q_lane_f32((const float*) exp3, exp3_v, 2);
            exp3 += expTableStride3;
            exp3_v = vld2q_lane_f32((const float*) exp3, exp3_v, 3);
            exp3 += expTableStride3;

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
}
#else
namespace ckfft
{
    void fft_neon(CkFftContextBase* context, const CkFftComplex* input, CkFftComplex* output, int count, int stride, int expTableDiv) {}
} 
#endif



