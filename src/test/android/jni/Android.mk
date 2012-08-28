LOCAL_PATH := $(call my-dir)

#################################################

include $(CLEAR_VARS)

LOCAL_MODULE := ckfft
LOCAL_SRC_FILES := ../../../ckfft/android/obj/local/$(TARGET_ARCH_ABI)/libckfft.a

include $(PREBUILT_STATIC_LIBRARY)

#################################################

include $(CLEAR_VARS)

LOCAL_MODULE := test
LOCAL_SRC_FILES := bindings.cpp ../../test.cpp
LOCAL_LDLIBS += -llog
LOCAL_CFLAGS += -Wno-psabi # fix warning about va_args in ndk r8b
LOCAL_STATIC_LIBRARIES := ckfft
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../../inc 

include $(BUILD_SHARED_LIBRARY)

