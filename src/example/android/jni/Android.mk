LOCAL_PATH := $(call my-dir)

#################################################

include $(CLEAR_VARS)

LOCAL_MODULE := example
LOCAL_SRC_FILES := bindings.cpp ../../main.cpp
LOCAL_LDLIBS += -llog
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../..
LOCAL_STATIC_LIBRARIES := ckfft 

include $(BUILD_SHARED_LIBRARY)

$(call import-add-path,$(LOCAL_PATH)/../../../ckfft/android)
$(call import-module,ckfft)
