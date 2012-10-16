#include <jni.h>
#include <android/log.h>
#include <string>
#include "../../test.h"

using namespace std;

namespace
{
    string g_externalStoragePath;
}

void getInputDir(string& dir)
{
    dir = g_externalStoragePath;
}

void getOutputDir(string& dir)
{
    dir = g_externalStoragePath;
}

extern "C"
{

void
Java_com_crickettechnology_ckfft_test_TestActivity_test(JNIEnv* jni, jobject thiz, jstring externalStoragePath)
{
    const char* pathStr = jni->GetStringUTFChars(externalStoragePath, NULL);
    g_externalStoragePath = pathStr;
    jni->ReleaseStringUTFChars(externalStoragePath, pathStr);

    // As on iOS, it appears that the thread is being interrupted for relatively long
    // periods of time during the test, which causes some very high timing results.
    // However, as far as I know, you cannot reliably increase the thread priority
    // on Android, so we're stuck with that.
    test();
}


}

