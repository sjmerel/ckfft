#include <jni.h>
#include <stddef.h>

int main(int argc, char* argv[]);

extern "C"
{

void
Java_com_crickettechnology_ckfft_example_ExampleActivity_main(JNIEnv* jni, jobject thiz)
{
    main(0, NULL);
}


}


