LOCAL_PATH := $(call my-dir)

#################################################

include $(CLEAR_VARS)

LOCAL_MODULE := ckfft
LOCAL_SRC_FILES := ../../ckfft.cpp
#LOCAL_CFLAGS += -Wno-psabi # fix warning about va_args in ndk r8b
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../../inc 

include $(BUILD_STATIC_LIBRARY)

