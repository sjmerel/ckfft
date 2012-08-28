#include <jni.h>
#include <android/log.h>
#include "../../test.h"

extern "C"
{

void
Java_com_crickettechnology_ckfft_test_TestActivity_test(JNIEnv* env, jobject thiz)
{
    test();
//    __android_log_write(ANDROID_LOG_INFO, "CKFFT", "hello!\n");
}


}
