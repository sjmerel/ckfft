#include <jni.h>
#include <android/log.h>
#include "../../test.h"

extern "C"
{

void
Java_com_crickettechnology_ckfft_test_TestActivity_test(JNIEnv* env, jobject thiz)
{
    test();
}


}
