#pragma once
#include "ckfft/platform.h"
#include <stdio.h>

#if CKFFT_PLATFORM_ANDROID
#  include <android/log.h>
#elif CKFFT_PLATFORM_WIN
#  include <stdarg.h>
#  include <windows.h>
#endif


////////////////////////////////////////
// print macro

#if CKFFT_PLATFORM_ANDROID
#  define CKFFT_PRINTF(fmt, ...) __android_log_print(ANDROID_LOG_INFO, "CKFFT", fmt, ##__VA_ARGS__)
#elif CKFFT_PLATFORM_WIN
#  define CKFFT_PRINTF(fmt, ...) debugPrintf(fmt, __VA_ARGS__)
#else
#  define CKFFT_PRINTF printf
#endif


#if CKFFT_PLATFORM_WIN
void debugPrintf(const char* fmt, ...)
{
    char buf[4096];
    va_list args;
    va_start(args, fmt);
    int n = _vsnprintf_s(buf, sizeof(buf), sizeof(buf)-1, fmt, args);
    va_end(args);

    buf[n] = '\0';
    OutputDebugString(buf);
    printf(buf);
}
#endif
