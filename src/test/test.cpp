#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <algorithm>

#include "ckfft/ckfft.h"
#include "ckfft/platform.h"
#include "ckfft/context.h"
#include "ckfft/debug.h"

#include "timer.h"
#include "stats.h"
#include "platform.h"

#if CKFFT_PLATFORM_MACOS || CKFFT_PLATFORM_IOS
#  include <Accelerate/Accelerate.h>
#endif

#if !CKFFT_PLATFORM_WIN
#  include <sys/utsname.h>
#endif

#include "kiss_fft130/kiss_fft.h"
#include "kiss_fft130/tools/kiss_fftr.h"
#include "tinyxml/tinyxml.h"

using namespace std;

////////////////////////////////////////

inline void CkFftVerify(int result)
{
    if (!result)
    {
        assert(false);
    }
}

////////////////////////////////////////

// print data
void print(const CkFftComplex* values, int count)
{
    for (int i = 0; i < count; ++i)
    {
        CkFftComplex value = values[i];
        CKFFT_PRINTF("%d: %f + %fi\n", i, value.real, value.imag);
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
        float diffR = value1.real - value2.real;
        float diffI = value1.imag - value2.imag;
        CKFFT_PRINTF("%d: %f + %fi , %f + %fi  (%f)\n", 
                i,
                value1.real, value1.imag, 
                value2.real, value2.imag, 
                diffR * diffR + diffI * diffI);
    }
}

// read data from file
void read(vector<CkFftComplex>& values, const char* path)
{
    // read input
    ifstream inFile;
    inFile.open(path);
    assert(inFile.good());

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
    for (int i = 0; i < (int) values.size(); ++i)
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
float compare(const CkFftComplex* values1, const CkFftComplex* values2, int count)
{
    // calculate rms of real and imaginary terms.
    // (would it be better to compare magnitudes of complex differences?)

    float sumSq = 0.0f;
    for (int i = 0; i < count; ++i)
    {
        const CkFftComplex& v1 = values1[i];
        const CkFftComplex& v2 = values2[i];

        float diffReal = v1.real - v2.real;
        sumSq += diffReal * diffReal;

        float diffImag = v1.imag - v2.imag;
        sumSq += diffImag * diffImag;
    }

    return sqrtf(sumSq / (count * 2));
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

class FftTester
{
public:
    FftTester() : 
        m_input(NULL),
        m_output(NULL),
        m_count(0), 
        m_inverse(false), 
        m_real(false) 
    {}

    virtual ~FftTester() {}

    virtual const char* getName() = 0;

    // initialize FFT code, prepare input & output
    void init(const CkFftComplex* input, CkFftComplex* output, int count, bool inverse, bool real)
    {
        m_input = input;
        m_output = output;
        m_count = count;
        m_inverse = inverse;
        m_real = real;

        initSub();
    }

    // do the FFT
    virtual void run()
    {
        runSub();
    }

    // convert output, shut down FFT code
    virtual void shutdown()
    {
        shutdownSub();
    }

protected:
    const CkFftComplex* m_input;
    CkFftComplex* m_output;
    int m_count;
    bool m_inverse;
    bool m_real;

    virtual void initSub() = 0;
    virtual void runSub() = 0;
    virtual void shutdownSub() = 0;

};

class CkFftTester : public FftTester
{
public:
    CkFftTester() : 
        FftTester(), 
        m_context(NULL), 
        m_maxCount(-1),
        m_tmpBuf(NULL)
    {}

    void setMaxCount(int maxCount) { m_maxCount = maxCount; }

    static void setNoNeon(bool noNeon) { s_noNeon = noNeon; }
    static bool isNeonEnabled()
    {
        return CkFftContext::isNeonSupported() && !s_noNeon;
    }

    virtual const char* getName() { return "ckfft"; }

protected:
    virtual void initSub()
    {
        int count = (m_maxCount >= 0 ? m_maxCount : m_count);
        m_context = CkFftInit(count, (m_inverse ? kCkFftDirection_Inverse : kCkFftDirection_Forward), NULL, NULL);
        if (s_noNeon)
        {
            m_context->neon = false;
        }

        if (m_real && m_inverse)
        {
            m_tmpBuf = new CkFftComplex[m_count/2 + 1];
        }
    }

    virtual void runSub()
    {
        if (m_real)
        {
            if (m_inverse)
            {
                CkFftVerify( CkFftRealInverse(m_context, m_count, m_input, (float*) m_output, m_tmpBuf) );
            }
            else
            {
                CkFftVerify( CkFftRealForward(m_context, m_count, (float*) m_input, m_output) );
            }
        }
        else
        {
            if (m_inverse)
            {
                CkFftVerify( CkFftComplexInverse(m_context, m_count, m_input, m_output) );
            }
            else
            {
                CkFftVerify( CkFftComplexForward(m_context, m_count, m_input, m_output) );
            }
        }
    }

    virtual void shutdownSub()
    {
        CkFftShutdown(m_context);
        m_context = NULL;
        delete[] m_tmpBuf;
        m_tmpBuf = NULL;
    }

private:
    CkFftContext* m_context;
    int m_maxCount;
    CkFftComplex* m_tmpBuf;

    static bool s_noNeon;
};

bool CkFftTester::s_noNeon = false;


// TODO: use fixed-point KISS?
class KissTester : public FftTester
{
public:
    KissTester() :
        m_cfg(NULL),
        m_realCfg(NULL)
    {}

    virtual const char* getName() { return "kiss_fft"; }

protected:
    virtual void initSub()
    {
        m_kissInput.resize(m_count);
        for (int i = 0; i < m_count; ++i)
        {
            m_kissInput[i].r = m_input[i].real;
            m_kissInput[i].i = m_input[i].imag;
        }

        m_kissOutput.resize(m_count);

        if (m_real)
        {
            m_realCfg = kiss_fftr_alloc(m_count, m_inverse, 0, 0);
        }
        else
        {
            m_cfg = kiss_fft_alloc(m_count, m_inverse, 0, 0);
        }
    }

    virtual void runSub()
    {
        if (m_real)
        {
            if (m_inverse)
            {
                kiss_fftri(m_realCfg, &m_kissInput[0], (float*) &m_kissOutput[0]);
            }
            else
            {
                kiss_fftr(m_realCfg, (float*) &m_kissInput[0], &m_kissOutput[0]);
            }
        }
        else
        {
            kiss_fft(m_cfg, &m_kissInput[0], &m_kissOutput[0]);
        }
    }

    virtual void shutdownSub()
    {
        float scale = 1.0f;
        if (m_real && !m_inverse)
        {
            scale = 2.0f;
        }

        for (int i = 0; i < m_count; ++i)
        {
            m_output[i].real = m_kissOutput[i].r * scale;
            m_output[i].imag = m_kissOutput[i].i * scale;
        }

        if (m_real)
        {
            free(m_realCfg);
        }
        else
        {
            free(m_cfg);
        }
    }

private:
    vector<kiss_fft_cpx> m_kissInput;
    vector<kiss_fft_cpx> m_kissOutput;
    kiss_fft_cfg m_cfg;
    kiss_fftr_cfg m_realCfg;
};

#if CKFFT_PLATFORM_IOS || CKFFT_PLATFORM_MACOS
class AccelerateTester : public FftTester
{
public:
    AccelerateTester() :
        m_logCount(0),
        m_setup(NULL)
    {}

    virtual const char* getName() { return "accelerate"; }

protected:
    virtual void initSub()
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

        if (m_real && m_inverse)
        {
            // input[0] and input[n/2] are real, so they pack input[n/2].real into
            // input[0].imag
            m_accInput.imagp[0] = m_accInput.realp[m_count/2];
        }

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

    virtual void runSub()
    {
        if (m_real)
        {
            vDSP_fft_zropt(
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
        else
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

    }

    virtual void shutdownSub()
    {
        for (int i = 0; i < m_count; ++i)
        {
            m_output[i].real = m_accOutputR[i];
            m_output[i].imag = m_accOutputI[i];
        }

        if (m_real && !m_inverse)
        {
            // output[0] and output[n/2] are real, so they pack output[n/2].real into
            // output[0].imag
            m_output[m_count/2].real = m_output[0].imag;
            m_output[m_count/2].imag = 0.0f;
            m_output[0].imag = 0.0f;
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

// build name of results file (e.g. results_ios_iPod_4_1.xml)
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

#if CKFFT_PLATFORM_WIN
    SYSTEM_INFO systemInfo;
    GetSystemInfo(&systemInfo);
    switch (systemInfo.wProcessorArchitecture)
    {
        case PROCESSOR_ARCHITECTURE_AMD64: name += "x64";  break;
        case PROCESSOR_ARCHITECTURE_ARM:   name += "arm";  break;
        case PROCESSOR_ARCHITECTURE_IA64:  name += "ia64"; break;
        case PROCESSOR_ARCHITECTURE_INTEL: name += "x86";  break;
        default: break;
    }
#else
    struct utsname systemInfo;
    uname(&systemInfo);

    name += systemInfo.machine;
#endif

    for (int i = 0; i < (int) name.size(); ++i)
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

#if CKFFT_PLATFORM_WIN
    CreateDirectory(dir.c_str(), NULL);
#else
    mkdir(dir.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
#endif

    string name;
    getResultsName(name);

    string path = dir + "/" + name;

    if (doc.SaveFile(path.c_str()))
    {
        CKFFT_PRINTF("\nwrote results to %s\n", path.c_str());
    }
}

////////////////////////////////////////

const int k_reps = 500;

void timingTest(const char* testName, 
          TiXmlDocument& doc,
          vector<FftTester*>& testers, 
          const vector<CkFftComplex>& input, 
          vector<CkFftComplex>& output, 
          bool inverse, 
          bool real)
{

    int count = (int) input.size();

    float baseTime = 0.0f;

    CKFFT_PRINTF("\n");
    CKFFT_PRINTF("%s:\n", testName);
    CKFFT_PRINTF("        name    median       min       max      norm    change\n");
    CKFFT_PRINTF("--------------------------------------------------------------------\n");

    // find XML element containing results for this test (or create one)
    TiXmlElement* rootElem = doc.FirstChildElement("ckfft_test");
    TiXmlElement* testElem = rootElem->FirstChildElement(testName);
    if (!testElem)
    {
        testElem = new TiXmlElement(testName);
        rootElem->LinkEndChild(testElem);
    }

    // note that testers[0] is ckfft
    for (int i = 0; i < (int) testers.size(); ++i)
    {
        // calculate output
        FftTester* tester = testers[i];
        tester->init(&input[0], &output[0], count, inverse, real);

        Stats stats(k_reps);
        for (int j = 0; j < k_reps; ++j)
        {
            Timer timer;
            timer.start();

            tester->run();

            timer.stop();
            float ms = timer.getElapsedMs();
            stats.sample(ms);
        }

        tester->shutdown();

        float time = stats.getMedian();

        if (i == 0)
        {
            baseTime = time;
        }

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

        testerElem->SetDoubleAttribute("time", time);

        CKFFT_PRINTF("%12s: %f  %f  %f  %f  %7.2f%%", tester->getName(), time, stats.getMin(), stats.getMax(), time/baseTime, 100.0f*(time - prevTime)/prevTime);
        CKFFT_PRINTF("\n");
    }
}

void timingTest()
{
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

    vector<FftTester*> testers;
    testers.push_back(new CkFftTester());
    testers.push_back(new KissTester());
#if CKFFT_PLATFORM_IOS || CKFFT_PLATFORM_MACOS
    testers.push_back(new AccelerateTester());
#endif

    CKFFT_PRINTF("\n%d iterations\n", k_reps);

    char testName[128];

    while (count > 128)
    {
        sprintf(testName, "fft_%d", count);
        timingTest(testName, doc, testers, input, output, false, false);

        sprintf(testName, "fft_inv_%d", count);
        timingTest(testName, doc, testers, output, invOutput, true, false);

        sprintf(testName, "fft_real_%d", count);
        timingTest(testName, doc, testers, input, output, false, true);

        sprintf(testName, "fft_inv_real_%d", count);
        timingTest(testName, doc, testers, output, invOutput, true, true);

        count /= 2;
        input.resize(count);
        output.resize(count);
        invOutput.resize(count);
    }

    writeResults(doc);

    for (int i = 0; i < (int) testers.size(); ++i)
    {
        delete testers[i];
    }

}

////////////////////////////////////////

bool regressionTestComplex(const CkFftComplex* input, int count, int maxCount, bool inverse)
{
    CkFftTester ckfft;
    KissTester kiss;

    // allocate output
    vector<CkFftComplex> ckfftOutput;
    ckfftOutput.resize(count);
    vector<CkFftComplex> kissOutput;
    kissOutput.resize(count);

    ckfft.setMaxCount(maxCount);
    ckfft.init(input, &ckfftOutput[0], count, inverse, false);
    ckfft.run();
    ckfft.shutdown();

    kiss.init(input, &kissOutput[0], count, inverse, false);
    kiss.run();
    kiss.shutdown();

    bool success = true;
    float err = compare(&ckfftOutput[0], &kissOutput[0], count);
    const float k_thresh = 0.001f; // threshold for RMS comparison
    CKFFT_PRINTF("count=%-5d, maxCount=%-5d, inverse=%d, neon=%d: err %f", count, maxCount, inverse, CkFftTester::isNeonEnabled(), err);
    if (err > k_thresh)
    {
        CKFFT_PRINTF("   ****** FAILED ******");
        success = false;
    }
    CKFFT_PRINTF("\n");

    return success;
}

bool regressionTestReal(const CkFftComplex* realInput, const float* floatInput, int count, int maxCount)
{
    bool success = true;
    CkFftTester ckfft;

    vector<CkFftComplex> refOutput;
    vector<CkFftComplex> output;

    refOutput.resize(count);
    output.resize(count);

    ckfft.setMaxCount(maxCount);

    // complex fft of real input
    ckfft.init(realInput, &refOutput[0], count, false, false);
    ckfft.run();
    ckfft.shutdown();

    // real fft of real (float) input
    ckfft.init((CkFftComplex*) floatInput, &output[0], count, false, true);
    ckfft.run();
    ckfft.shutdown();

    int outputCount = count/2+1;

    // compensate for different scale factors
    for (int i = 0; i < outputCount; ++i)
    {
        output[i].real *= 0.5f;
        output[i].imag *= 0.5f;
    }
//        print(output, refOutput, outputCount);

    float err = compare(&output[0], &refOutput[0], outputCount);
    const float k_thresh = 0.001f; // threshold for RMS comparison
    CKFFT_PRINTF("count=%-5d, maxCount=%-5d, neon=%d, forward real: err %f", count, maxCount, CkFftTester::isNeonEnabled(), err);
    if (err > k_thresh)
    {
        CKFFT_PRINTF("   ****** FAILED ******");
        success = false;
    }
    CKFFT_PRINTF("\n");


    // inverse
    vector<CkFftComplex> invRefOutput;
    vector<float> floatOutput;

    invRefOutput.resize(count);
    floatOutput.resize(count);

    ckfft.init(&refOutput[0], &invRefOutput[0], count, true, false);
    ckfft.run();
    ckfft.shutdown();

    ckfft.init(&refOutput[0], (CkFftComplex*) &floatOutput[0], count, true, true);
    ckfft.run();
    ckfft.shutdown();

    float sumSq = 0.0f;
    for (int i = 0; i < outputCount; ++i)
    {
        float diff = floatOutput[i] - invRefOutput[i].real;
        sumSq += diff * diff;
    }

    err = sqrtf(sumSq / outputCount);
    CKFFT_PRINTF("count=%-5d, maxCount=%-5d, neon=%d, inverse real: err %f", count, maxCount, CkFftTester::isNeonEnabled(), err);
    if (err > k_thresh)
    {
        CKFFT_PRINTF("   ****** FAILED ******");
        success = false;
    }
    CKFFT_PRINTF("\n");

    return success;
}

bool regressionTest()
{
    // read input
    vector<CkFftComplex> input;
    string path;
    getInputDir(path);
    path += "/input.txt";
    read(input, path.c_str());
    int count = (int) input.size();

    bool success = true;

    // compare complex forward & inverse against kiss_fft
    CKFFT_PRINTF("complex FFTs:\n");
    int maxCount = count*2;
    while (count > 0)
    {
        // complex forward
        success &= regressionTestComplex(&input[0], count, count, false);
        // same with maxCount > count
        success &= regressionTestComplex(&input[0], count, maxCount, false);

        // complex inverse
        success &= regressionTestComplex(&input[0], count, count, true);
        // same with maxCount > count
        success &= regressionTestComplex(&input[0], count, maxCount, true);

        count /= 2;
    }


    // compare results of real fft with results of complex fft on real input
    CKFFT_PRINTF("\n");
    CKFFT_PRINTF("real FFTs:\n");
    vector<CkFftComplex> realInput = input;
    vector<float> floatInput;
    floatInput.resize(realInput.size());
    for (int i = 0; i < (int) realInput.size(); ++i)
    {
        realInput[i].imag = 0.0f;
        floatInput[i] = realInput[i].real;
    }

    count = (int) realInput.size();
    while (count > 0)
    {
        // real forward and inverse
        success &= regressionTestReal(&realInput[0], &floatInput[0], count, count);
        // same with maxCount > count
        success &= regressionTestReal(&realInput[0], &floatInput[0], count, maxCount);

        count /= 2;
    }

    return success;
}


////////////////////////////////////////

bool test()
{
    Timer::init();
//    writeInput(4096);

    bool success = true;
    success &= regressionTest();
#if CKFFT_ARM_NEON
    CkFftTester::setNoNeon(true);
    success &= regressionTest();
    CkFftTester::setNoNeon(false);
#endif

    if (!success)
    {
        CKFFT_PRINTF("*********** SOME TESTS FAILED *************\n");
    }
    else
    {
        timingTest();
    }

    return success;
}

