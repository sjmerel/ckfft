#include <iostream>
#include <fstream>
#include <vector>
#include <assert.h>
#include <math.h>

#include "ckfft/ckfft.h"

//#include "kiss_fft129/kiss_fft.h"

#if __APPLE__
#  include <TargetConditionals.h>
#endif
#if TARGET_OS_IPHONE
#  include "ios/helpers.h"
#endif

using namespace std;

void print(const vector<CkFftComplex>& values)
{
    for (int i = 0; i < values.size(); ++i)
    {
        CkFftComplex value = values[i];
        printf("%f + %f i\n", value.real, value.imag);
    }
}

void read(vector<CkFftComplex>& values, const char* path)
{
    // read input
    ifstream inFile;
    inFile.open(path);
    CkFftComplex value;
    while (inFile.good() && !inFile.eof())
    {
        inFile >> value.real;
        inFile >> value.imag;
        if (!inFile.eof())
        {
            values.push_back(value);
        }
    }
}

void write(const vector<CkFftComplex>& values, const char* path)
{
    ofstream outFile;
    outFile.open(path);
    CkFftComplex value;
    for (int i = 0; i < values.size(); ++i)
    {
        value = values[i];
        outFile << value.real;
        outFile << value.imag;
    }

}

////////////////////////////////////////

void test()
{
    /*
    // generate input

    ofstream outFile;
    outFile.open("input.txt");
    CkFftComplex value;
    for (int i = 0; i < 512; ++i)
    {
        value.real = (rand() - (RAND_MAX / 2)) / (float) (RAND_MAX / 2);
        value.imag = (rand() - (RAND_MAX / 2)) / (float) (RAND_MAX / 2);

        outFile << value.real << std::endl;
        outFile << value.imag << std::endl;
    }
    outFile.close();
    */

    ////////////////////////////////////////

    CkFftInit();

    vector<CkFftComplex> input;
    vector<CkFftComplex> output;

    // read input
    string path("input.txt");
#if TARGET_OS_IPHONE
    makeBundlePath(path);
#endif
    read(input, path.c_str());

    printf("input:\n");
    printf("-------\n");
    print(input);

    // process
    output.resize(input.size());
    CkFft(&input[0], &output[0], (int) input.size());

    // write output
    write(output, "output.txt");

    printf("\n");
    printf("output:\n");
    printf("-------\n");
    print(output);

    CkFftShutdown();

    ////////////////////////////////////////

    /*
    // process reference
    kiss_fft_cfg cfg = kiss_fft_alloc(input.size(), false, 0, 0);
    vector<kiss_fft_cpx> input2;
    vector<kiss_fft_cpx> output2;
    input2.resize(input.size());
    output2.resize(input.size());
    for (int i = 0; i < input.size(); ++i)
    {
        input2[i].r = input[i].real;
        input2[i].i = input[i].imag;
    }
    kiss_fft(cfg, &input2[0], &output2[0]);
    free(cfg);

    printf("\n");
    printf("reference:\n");
    printf("-------\n");
    for (int i = 0; i < output2.size(); ++i)
    {
        printf("%f + %f i\n", output2[i].r, output2[i].i);
    }
    */

    ////////////////////////////////////////

    printf("done!\n");
}


