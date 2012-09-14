#pragma once
#include "ckfft/platform.h"
#include <stdio.h>


////////////////////////////////////////
// print macro

#if CKFFT_PLATFORM_ANDROID
#  include <android/log.h>
#  define CKFFT_PRINTF(fmt, ...) __android_log_print(ANDROID_LOG_INFO, "CKFFT", fmt, ##__VA_ARGS__)
#else
#  define CKFFT_PRINTF printf
#endif


