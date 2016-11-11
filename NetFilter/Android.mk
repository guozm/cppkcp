LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_CFLAGS := -D__STDC_CONSTANT_MACROS

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/src \
	$(LOCAL_PATH)/../Net/inc

LOCAL_SRC_FILES := \
	src/netfilter.cpp \
	src/tpktfilter.cpp 

#LOCAL_LDLIBS := -lm -llog

LOCAL_MODULE := libV2NetFilter

#include $(BUILD_SHARED_LIBRARY)
include $(BUILD_STATIC_LIBRARY)
