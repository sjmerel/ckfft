LOCAL_PATH := $(call my-dir)

#################################################

include $(CLEAR_VARS)

LOCAL_MODULE := ckfft
LOCAL_SRC_FILES := ckfft.cpp
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../inc 
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../inc

include $(BUILD_STATIC_LIBRARY)

