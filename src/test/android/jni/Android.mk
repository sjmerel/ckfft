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

LOCAL_LDLIBS += -llog
LOCAL_CFLAGS += -Wno-psabi # fix warning about va_args in ndk r8b
LOCAL_C_INCLUDES := \
   $(LOCAL_PATH)/../../../ \
   $(LOCAL_PATH)/../../../../ext
LOCAL_STATIC_LIBRARIES := ckfft 

include $(BUILD_SHARED_LIBRARY)

# NOTE:
#  This call assumes that the NDK_MODULE_PATH environment variable includes the path 
# to src/ckfft.  You can set that environment variable yourself; or you can set it when
# you invoke ndk-build; or you can use the makefile in the src/test/android directory.

$(call import-module,ckfft)
