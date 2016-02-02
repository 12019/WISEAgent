LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# My macros define 
MY_C_FILE_LIST			:= $(wildcard $(LOCAL_PATH)/*.c)

# Module
LOCAL_MODULE		 	:= cagent 
LOCAL_C_INCLUDES		:= $(LOCAL_PATH)/../../Include 
LOCAL_SRC_FILES			:= $(MY_C_FILE_LIST:$(LOCAL_PATH)/%=%)
LOCAL_CFLAGS 			:= -fPIC

LOCAL_STATIC_LIBRARIES	:= \
	advcarehelper \
	SAGatherInfo \
	SAClient \
	SAConfig \
	Platform \
	Log 

LOCAL_LDLIBS 			:= -ldl

# Build 
include $(BUILD_EXECUTABLE)


