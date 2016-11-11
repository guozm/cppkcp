LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_CFLAGS += -DPRINT_LOG

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/inc \
	$(LOCAL_PATH)/src \
	$(LOCAL_PATH)/../ThreadPool/inc \
	$(LOCAL_PATH)/../libevent_android/inc \
	$(LOCAL_PATH)/../V2_Log/inc \
	$(LOCAL_PATH)/../Xml/inc \
	$(LOCAL_PATH)/../Expat/inc \
	$(LOCAL_PATH)/../udx/include \
	$(LOCAL_PATH)/../../Inc \
	$(LOCAL_PATH)/../ikcp


LOCAL_SRC_FILES := \
	src/AppModeThread.cpp \
	src/BaseIOThread.cpp \
	src/IOService.cpp \
	src/IoUtils.cpp \
	src/Mutex.cpp \
	src/networkio.cpp \
	src/Session.cpp \
	src/TCPReadThread.cpp \
	src/TCPWriteThread.cpp \
	src/UdxTcpSession.cpp \
	src/UdxTcpHandler.cpp \
	src/KCPClient.cpp \
	src/KCPReadThread.cpp \
	src/KCPServer.cpp \
	src/KCPThread.cpp \
	src/KCPWriteThread.cpp \
	src/kcpinc.cpp \
	../ikcp/ikcp.c

LOCAL_CPPFLAGS += -DLINUX

LOCAL_MODULE := libNetEvent

include $(BUILD_STATIC_LIBRARY)


