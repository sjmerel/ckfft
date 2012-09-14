#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/utsname.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <algorithm>

#include "ckfft/ckfft.h"
#include "ckfft/platform.h"
#include "ckfft/debug.h"

#include "timer.h"
#include "stats.h"
#include "platform.h"

#if CKFFT_PLATFORM_MACOS || CKFFT_PLATFORM_IOS
#  include <Accelerate/Accelerate.h>
#endif

#include "kiss_fft130/kiss_fft.h"
#include "tinyxml/tinyxml.h"



using namespace std;

////////////////////////////////////////

// print data
void print(const CkFftComplex* values, int count)
{
    for (int i = 0; i < count; ++i)
    {
        CkFftComplex value = values[i];
        CKFFT_PRINTF("%f + %f i\n", value.real, value.imag);
    }
}

void print(const vector<CkFftComplex>& values, int count = 0)
{
    if (count <= 0)
    {
        count = (int) values.size();
    }
    print(&values[0], count);
}

// print two data sets for side-by-side comparison
void print(const vector<CkFftComplex>& values1, const vector<CkFftComplex>& values2, int count = 0)
{
    assert(values1.size() == values2.size());
    if (count <= 0)
    {
        count = (int) values1.size();
    }
    for (int i = 0; i < count; ++i)
    {
        CkFftComplex value1 = values1[i];
        CkFftComplex value2 = values2[i];
        CKFFT_PRINTF("%f/%f + %f/%f i\n", value1.real, value2.real, value1.imag, value2.imag);
    }
}

// read data from file
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

// write data to file
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

// set data to 0
void zero(CkFftComplex* values, int count)
{
    for (int i = 0; i < count; ++i)
    {
        values[i].real = 0.0f;
        values[i].imag = 0.0f;
    }
}

void zero(vector<CkFftComplex>& values)
{
    zero(&values[0], (int) values.size());
}

// compare two data sets; return RMS difference of all components
float compare(const vector<CkFftComplex>& values1, const vector<CkFftComplex>& values2)
{
    // calculate rms of real and imaginary terms.
    // (would it be better to compare magnitudes of complex differences?)
    assert(values1.size() == values2.size());
    assert(!values1.empty());
    float sumSq = 0.0f;
    for (int i = 0; i < values1.size(); ++i)
    {
        const CkFftComplex& v1 = values1[i];
        const CkFftComplex& v2 = values2[i];

        float diffReal = v1.real - v2.real;
        sumSq += diffReal * diffReal;

        float diffImag = v1.imag - v2.imag;
        sumSq += diffImag * diffImag;
    }

    return sqrtf(sumSq / (values1.size() * 2));
}

// generate input file
void writeInput(int count)
{
    ofstream outFile;
    outFile.open("input.txt");
    CkFftComplex value;
    for (int i = 0; i < count; ++i)
    {
        value.real = (rand() - (RAND_MAX / 2)) / (float) (RAND_MAX / 2);
        value.imag = (rand() - (RAND_MAX / 2)) / (float) (RAND_MAX / 2);

        outFile << value.real << std::endl;
        outFile << value.imag << std::endl;
    }
    outFile.close();
}


////////////////////////////////////////

const int k_reps = 500;

class FftTester
{
public:
    FftTester() : m_input(NULL), m_output(NULL), m_count(0), m_inverse(false) {}

    virtual ~FftTester() {}

    virtual const char* getName() = 0;

    virtual float test(const CkFftComplex* input, CkFftComplex* output, int count, bool inverse)
    {
        m_input = input;
        m_output = output;
        m_count = count;
        m_inverse = inverse;
        zero(output, count);

        init();

        for (int i = 0; i < k_reps; ++i)
        {
            Timer timer;
            timer.start();

            fft();

            timer.stop();
            m_stats.sample(timer.getElapsedMs());
        }

        shutdown();

        return m_stats.getMean();
    }

protected:
    const CkFftComplex* m_input;
    CkFftComplex* m_output;
    int m_count;
    bool m_inverse;

    virtual void init() = 0;
    virtual void fft() = 0;
    virtual void shutdown() = 0;

private:
    Stats m_stats;
};

class CkFftTester : public FftTester
{
public:
    virtual const char* getName() { return "ckfft"; }

protected:
    virtual void init()
    {
        m_context = CkFftInit(m_count);
    }

    virtual void fft()
    {
        if (m_inverse)
        {
            CkInvFft(m_context, m_input, m_output, m_count);
        }
        else
        {
            CkFft(m_context, m_input, m_output, m_count);
        }
    }

    virtual void shutdown()
    {
        CkFftShutdown(m_context);
    }

private:
    CkFftContext* m_context;
};

// TODO: use fixed-point KISS?
class KissTester : public FftTester
{
public:
    virtual const char* getName() { return "kiss_fft"; }

protected:
    virtual void init()
    {
        m_kissInput.resize(m_count);
        for (int i = 0; i < m_count; ++i)
        {
            m_kissInput[i].r = m_input[i].real;
            m_kissInput[i].i = m_input[i].imag;
        }

        m_kissOutput.resize(m_count);

        m_cfg = kiss_fft_alloc(m_count, m_inverse, 0, 0);
    }

    virtual void fft()
    {
        kiss_fft(m_cfg, &m_kissInput[0], &m_kissOutput[0]);
    }

    virtual void shutdown()
    {
        for (int i = 0; i < m_count; ++i)
        {
            m_output[i].real = m_kissOutput[i].r;
            m_output[i].imag = m_kissOutput[i].i;
        }

        free(m_cfg);
    }

private:
    vector<kiss_fft_cpx> m_kissInput;
    vector<kiss_fft_cpx> m_kissOutput;
    kiss_fft_cfg m_cfg;
};

class LibAVTester : public FftTester
{
public:
    virtual const char* getName() { return "libav"; }

protected:
    virtual void init()
    {
    }

    virtual void fft()
    {
    }

    virtual void shutdown()
    {
    }

private:
};

#if CKFFT_PLATFORM_IOS || CKFFT_PLATFORM_MACOS
class AccelerateTester : public FftTester
{
public:
    virtual const char* getName() { return "accelerate"; }

protected:
    virtual void init()
    {
        m_accInputR.resize(m_count);
        m_accInputI.resize(m_count);
        for (int i = 0; i < m_count; ++i)
        {
            m_accInputR[i] = m_input[i].real;
            m_accInputI[i] = m_input[i].imag;
        }
        m_accInput.realp = &m_accInputR[0];
        m_accInput.imagp = &m_accInputI[0];

        m_accOutputR.resize(m_count);
        m_accOutputI.resize(m_count);
        m_accOutput.realp = &m_accOutputR[0];
        m_accOutput.imagp = &m_accOutputI[0];

        // malloc returns 16-byte alignment
        int tempBytes = std::min(16384, (int) (m_count * sizeof(float)));
        m_accTemp.realp = (float*) malloc(tempBytes);
        m_accTemp.imagp = (float*) malloc(tempBytes);
        assert(((size_t) m_accTemp.realp) % 16 == 0);
        assert(((size_t) m_accTemp.imagp) % 16 == 0);

        // determine log2(m_count)
        m_logCount = 0;
        int count = m_count;
        while (count > 1)
        {
            ++m_logCount;
            count >>= 1;
        }

        m_setup = vDSP_create_fftsetup(m_logCount, kFFTRadix2);
    }

    virtual void fft()
    {
        vDSP_fft_zopt(
                m_setup,//FFTSetup __vDSP_setup,
                &m_accInput, // DSPSplitComplex *__vDSP_signal,
                1, //vDSP_Stride __vDSP_signalStride,
                &m_accOutput, //DSPSplitComplex *__vDSP_result,
                1, //vDSP_Stride __vDSP_strideResult,
                &m_accTemp,
                m_logCount, //vDSP_Length __vDSP_log2n,
                (m_inverse ? kFFTDirection_Inverse : kFFTDirection_Forward) //FFTDirection __vDSP_direction
                );

    }

    virtual void shutdown()
    {
        for (int i = 0; i < m_count; ++i)
        {
            m_output[i].real = m_accOutputR[i];
            m_output[i].imag = m_accOutputI[i];
        }
        free(m_accTemp.realp);
        free(m_accTemp.imagp);
        vDSP_destroy_fftsetup(m_setup);
    }

private:
    vector<float> m_accInputR;
    vector<float> m_accInputI;
    DSPSplitComplex m_accInput;
    vector<float> m_accOutputR;
    vector<float> m_accOutputI;
    DSPSplitComplex m_accOutput;
    DSPSplitComplex m_accTemp;
    int m_logCount;
    FFTSetup m_setup;
};
#endif

////////////////////////////////////////

void getResultsName(string& name)
{
    name = "results_";

#if CKFFT_PLATFORM_IOS
    name += "ios";
#elif CKFFT_PLATFORM_ANDROID
    name += "android";
#elif CKFFT_PLATFORM_MACOS
    name += "macos";
#elif CKFFT_PLATFORM_WIN
    name += "win";
#endif

    name += "_";

    struct utsname systemInfo;
    uname(&systemInfo);

    name += systemInfo.machine;

    for (int i = 0; i < name.size(); ++i)
    {
        if (name[i] == ',')
        {
            name[i] = '_';
        }
    }

    name += ".xml";
}

void readResults(TiXmlDocument& doc)
{
    string dir;
    getInputDir(dir);

    string name;
    getResultsName(name);

    string path = dir + "/" + name;

    if (doc.LoadFile(path.c_str()))
    {
        CKFFT_PRINTF("read results from %s\n", path.c_str());
    }
    else
    {
        doc.LinkEndChild(new TiXmlElement("ckfft_test"));
    }
}

void writeResults(const TiXmlDocument& doc)
{
    string dir;
    getOutputDir(dir);
    dir += "/out";

    mkdir(dir.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);

    string name;
    getResultsName(name);

    string path = dir + "/" + name;

    if (doc.SaveFile(path.c_str()))
    {
        CKFFT_PRINTF("\nwrote results to %s\n", path.c_str());
    }
}

////////////////////////////////////////

void test(const char* testName, 
          TiXmlDocument& doc,
          vector<FftTester*>& testers, 
          const vector<CkFftComplex>& input, 
          vector<CkFftComplex>& output, 
          bool inverse)
{
    const float k_thresh = 0.0001f; // threshold for RMS comparison

    int count = (int) input.size();

    // allocate reference output
    vector<CkFftComplex> refOutput;
    refOutput.resize(count);

    float baseTime;

    CKFFT_PRINTF("\n");
    CKFFT_PRINTF("%s:\n", testName);
    CKFFT_PRINTF("        name       err      time      norm    change\n");
    CKFFT_PRINTF("----------------------------------------------------\n");

    // find XML element containing results for this test (or create one)
    TiXmlElement* rootElem = doc.FirstChildElement("ckfft_test");
    TiXmlElement* testElem = rootElem->FirstChildElement(testName);
    if (!testElem)
    {
        testElem = new TiXmlElement(testName);
        rootElem->LinkEndChild(testElem);
    }

    // note that testers[0] is ckfft
    for (int i = 0; i < testers.size(); ++i)
    {
        // calculate output
        FftTester* tester = testers[i];
        float time = tester->test(&input[0], (i == 0 ? &output[0] : &refOutput[0]), count, inverse);

        if (i == 0)
        {
            baseTime = time;
        }

        float err = (i == 0 ? 0.0f : compare(output, refOutput));

        float prevTime = time;

        // find XML element for containing results for this tester
        TiXmlElement* testerElem = testElem->FirstChildElement(tester->getName());
        if (testerElem)
        {
            testerElem->QueryFloatAttribute("time", &prevTime);
        }
        else
        {
            testerElem = new TiXmlElement(tester->getName());
            testElem->LinkEndChild(testerElem);
        }

        testerElem->SetDoubleAttribute("error", err);
        testerElem->SetDoubleAttribute("time", time);

        CKFFT_PRINTF("%12s: %f  %f  %f  %7.2f%%", tester->getName(), err, time, time/baseTime, 100.0f*(time - prevTime)/prevTime);
        if (err > k_thresh)
        {
            CKFFT_PRINTF("   ****** FAILED ******");
        }
        CKFFT_PRINTF("\n");
    }
}

void test()
{
    Timer::init();

    vector<FftTester*> testers;
    testers.push_back(new CkFftTester());
    testers.push_back(new KissTester());
//    testers.push_back(new LibAVTester());
#if CKFFT_PLATFORM_IOS || CKFFT_PLATFORM_MACOS
    testers.push_back(new AccelerateTester());
#endif

    // read input
    vector<CkFftComplex> input;
    string path;
    getInputDir(path);
    path += "/input.txt";
    read(input, path.c_str());
    int count = (int) input.size();

    // allocate output
    vector<CkFftComplex> output;
    output.resize(count);

    // allocate inverse output
    vector<CkFftComplex> invOutput;
    invOutput.resize(count);

    // read results xml
    TiXmlDocument doc;
    readResults(doc);

    CKFFT_PRINTF("\n%d iterations\n", k_reps);

    char testName[128];

    // TODO other sizes of FFT

    sprintf(testName, "fft_%d", count);
    test(testName, doc, testers, input, output, false);

    sprintf(testName, "invfft_%d", count);
    test(testName, doc, testers, output, invOutput, true);

    writeResults(doc);

    for (int i = 0; i < testers.size(); ++i)
    {
        delete testers[i];
    }
}


