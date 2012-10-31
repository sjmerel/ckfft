#pragma once
#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif


typedef struct
{
    float real;
    float imag;
}
CkFftComplex;


typedef struct _CkFftContext CkFftContext;


typedef enum 
{
    kCkFftDirection_Forward = (1 << 0),
    kCkFftDirection_Inverse = (1 << 1),
    kCkFftDirection_Both    = (kCkFftDirection_Forward | kCkFftDirection_Inverse)
}
CkFftDirection;


// Create an FFT context.
//
// Parameters:
//   count:   Number of elements in the FFT; must be a power of 2.
//   inverse: 0 for a forward FFT, != 0 for an inverse FFT.
//   buf:     Optional memory buffer.
//   bufSize: Optional pointer to size of memory buffer, in bytes.
//
// If you are content to let CkFftInit() allocate its own memory, pass in NULL for
// both buf and bufSize.  
//
// If buf is not NULL, and the value pointed to by bufSize is large enough, then
// then the FFT will use buf for its memory.  If the value pointed by bufSize is not
// large enough, then the minimum number of bytes required will be placed in *bufSize.
// 
// For example:
//   size_t memSize = 0;
//   CkFftInit(count, inverse, NULL, &memSize);
//   void* mem = malloc(memSize);
//   CkFftContext* context = CkFftInit(count, inverse, mem, &memSize);
//
// Returns a context pointer if one could be created, or NULL if not.
//
CkFftContext* CkFftInit(int maxCount, CkFftDirection direction, void* buf, size_t* bufSize);



// Perform the FFT.
//
// Parameters:
//   context: A context pointer from CkFftInit().
//   input:   Input data; the number of elements should be the value of count passed to CkFftInit().
//   output:  Output data; the number of elements should be the same as for the input data.
//
// The FFT is NOT performed in-place, so input and output should be different buffers.
// If your input or output data are laid out in memory appropriately (e.g. in an array
// of structs equivalent to CkFftComplex, or a float array with real and imaginary
// values interleaved), then a simple cast to CkFftComplex* will suffice:
//
//  CkFft(context, (const CkFftComplex*) input, (CkFftComplex*) output);
// 
// No scaling is applied to the results of either the forward or inverse FFT, so if you 
// apply a forward FFT followed by an inverse FFT to a set of data, the result is
// the original data, scaled by count.
// 
// Returns 1 if the FFT could be performed, or 0 if an error occurred (e.g. if the
// context was NULL or if input == output).
//

int CkFftRealForward(CkFftContext* context, int count, const float* input, CkFftComplex* output);
int CkFftRealInverse(CkFftContext* context, int count, const CkFftComplex* input, float* output, void* tmpBuf, size_t* tmpBufSize);
int CkFftComplexForward(CkFftContext* context, int count, const CkFftComplex* input, CkFftComplex* output);
int CkFftComplexInverse(CkFftContext* context, int count, const CkFftComplex* input, CkFftComplex* output);


// Destroy an FFT context.
//
// If you let CkFftInit() allocate its own memory buffer, then this will free that buffer.
//
void CkFftShutdown(CkFftContext*);








#ifdef __cplusplus
} // extern "C"
#endif
