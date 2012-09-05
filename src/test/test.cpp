#include <assert.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/utsname.h>

#if CKFFT_PLATFORM_WIN
#  define unlink _unlink
#  define S_IFREG _S_IFREG
#else
#  include <unistd.h>
#endif

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>

#include "ckfft/ckfft.h"
#include "ckfft/platform.h"
#include "ckfft/debug.h"

#include "timer.h"
#include "stats.h"
#include "platform.h"

#include "kiss_fft130/kiss_fft.h"
#include "tinyxml/tinyxml.h"



using namespace std;

////////////////////////////////////////

// print data
void print(const vector<CkFftComplex>& values)
{
    for (int i = 0; i < values.size(); ++i)
    {
        CkFftComplex value = values[i];
        CKFFT_PRINTF("%f + %f i\n", value.real, value.imag);
    }
}

// print two data sets for side-by-side comparison
void print(const vector<CkFftComplex>& values1, const vector<CkFftComplex>& values2)
{
    assert(values1.size() == values2.size());
    for (int i = 0; i < values1.size(); ++i)
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
void zero(vector<CkFftComplex>& values)
{
    for (int i = 0; i < values.size(); ++i)
    {
        values[i].real = 0.0f;
        values[i].imag = 0.0f;
    }
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
        CkFftInit();
    }

    virtual void fft()
    {
        if (m_inverse)
        {
            CkInvFft(m_input, m_output, m_count);
        }
        else
        {
            CkFft(m_input, m_output, m_count);
        }
    }

    virtual void shutdown()
    {
        CkFftShutdown();
    }
};

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

////////////////////////////////////////

struct Results
{
    float error;
    float time;
};

typedef map<string, Results> ResultsMap;

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

void readResults(ResultsMap& resultsMap)
{
    string dir;
    getInputDir(dir);

    string name;
    getResultsName(name);

    string path = dir + "/" + name;

    TiXmlDocument doc;
    if (doc.LoadFile(path.c_str()))
    {
        TiXmlElement* elem = doc.FirstChildElement("ckfft_test")->FirstChildElement();
        while (elem)
        {
            Results results;
            elem->QueryFloatAttribute("error", &results.error);
            elem->QueryFloatAttribute("time", &results.time);
            resultsMap[elem->Value()] = results;
            elem = elem->NextSiblingElement();
        }

        CKFFT_PRINTF("read results from %s\n", path.c_str());
    }
}

void writeResults(const ResultsMap& resultsMap)
{
    TiXmlDocument resultsDoc;
    TiXmlElement* rootElem = new TiXmlElement("ckfft_test");
    resultsDoc.LinkEndChild(rootElem);

    for (ResultsMap::const_iterator it = resultsMap.begin(); it != resultsMap.end(); ++it)
    {
        TiXmlElement* elem = new TiXmlElement(it->first.c_str());
        elem->SetDoubleAttribute("error", it->second.error);
        elem->SetDoubleAttribute("time", it->second.time);
        rootElem->LinkEndChild(elem);
    }

    string dir;
    getOutputDir(dir);
    dir += "/out";

    mkdir(dir.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);

    string name;
    getResultsName(name);

    string path = dir + "/" + name;

    if (resultsDoc.SaveFile(path.c_str()))
    {
        CKFFT_PRINTF("wrote results to %s\n", path.c_str());
    }
}

////////////////////////////////////////

void test(vector<FftTester*>& testers, ResultsMap& resultsMap, const ResultsMap& prevResultsMap)
{
    const float k_thresh = 0.0001f; // threshold for RMS comparison

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
    zero(output);

    // allocate reference output
    vector<CkFftComplex> refOutput;
    refOutput.resize(count);
    zero(refOutput);

    float baseTime;

    CKFFT_PRINTF("    name       err      time      norm    change\n");
    CKFFT_PRINTF("------------------------------------------------\n");

    // note that testers[0] is ckfft
    for (int i = 0; i < testers.size(); ++i)
    {
        // calculate output
        FftTester* tester = testers[i];
        float time = tester->test(&input[0], (i == 0 ? &output[0] : &refOutput[0]) , count, false);
        if (i == 0)
        {
            baseTime = time;
        }

        float err = (i == 0 ? 0.0f : compare(output, refOutput));
        if (err > k_thresh)
        {
            CKFFT_PRINTF("   ****** FAILED ******");
        }

        Results results;
        results.error = err;
        results.time = time;
        resultsMap[tester->getName()] = results;

        Results prevResults = results;
        ResultsMap::const_iterator it = prevResultsMap.find(tester->getName());
        if (it != prevResultsMap.end())
        {
            prevResults = it->second;
        }

        CKFFT_PRINTF("%8s: %f  %f  %f  %7.2f%%", tester->getName(), err, time, time/baseTime, (time - prevResults.time)/prevResults.time);
        CKFFT_PRINTF("\n");
    }


    /*
    // allocate inverse output
    vector<CkFftComplex> invOutput;
    invOutput.resize(count);
    zero(invOutput);

    // calculate inverse output
    testCkFft(output, invOutput, true);

    // allocate reference inverse output
    vector<CkFftComplex> refInvOutput;
    refInvOutput.resize(count);
    zero(refInvOutput);

    // calculate reference inverse output
    testKissFft(refOutput, refInvOutput, true);

    rms = compare(output, refOutput);
    CKFFT_PRINTF("inverse vs reference: %f\n", rms);
    if (rms > k_thresh)
    {
        return false;
    }

    rms = compare(input, invOutput);
    CKFFT_PRINTF("inverse vs input: %f\n", rms);
    if (rms > k_thresh)
    {
        return false;
    }
    */
}

void test()
{
    Timer::init();

    vector<FftTester*> testers;
    testers.push_back(new CkFftTester());
    testers.push_back(new KissTester());

    ResultsMap prevResultsMap;
    readResults(prevResultsMap);
    CKFFT_PRINTF("\n");

    ResultsMap resultsMap;
    test(testers, resultsMap, prevResultsMap);

    CKFFT_PRINTF("\n");
    writeResults(resultsMap);

    for (int i = 0; i < testers.size(); ++i)
    {
        delete testers[i];
    }

    CKFFT_PRINTF("done!\n");
}


