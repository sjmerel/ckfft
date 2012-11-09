LOCAL_PATH := $(call my-dir)

#################################################

include $(CLEAR_VARS)

LOCAL_MODULE := test
LOCAL_SRC_FILES := \
   platform.cpp \
   ../../test.cpp \
   ../../stats.cpp \
   ../../timer.cpp \
   ../../timer_android.cpp \
   ../../../../ext/tinyxml/tinyxml.cpp \
   ../../../../ext/tinyxml/tinystr.cpp \
   ../../../../ext/tinyxml/tinyxmlerror.cpp \
   ../../../../ext/tinyxml/tinyxmlparser.cpp \
   ../../../../ext/kiss_fft130/kiss_fft.c \
   ../../../../ext/kiss_fft130/tools/kiss_fftr.c \

LOCAL_LDLIBS += -llog
LOCAL_CFLAGS += -Wno-psabi # fix warning about va_args in ndk r8b
LOCAL_C_INCLUDES := \
   $(LOCAL_PATH)/../../../ \
   $(LOCAL_PATH)/../../../../ext \
   $(LOCAL_PATH)/../../../../ext/kiss_fft130
LOCAL_STATIC_LIBRARIES := ckfft 

include $(BUILD_SHARED_LIBRARY)

$(call import-add-path,../../ckfft/android)
$(call import-module,ckfft)


