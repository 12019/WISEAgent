LOCAL_PATH				:= $(call my-dir)

# define my vars
ifeq ($(NDK_DEBUG), 1)
	MY_SRC_FIELS		:= libmosquitto-debug.a
else
	MY_SRC_FIELS		:= libmosquitto.a
endif 

include $(CLEAR_VARS)
LOCAL_MODULE			:= mosquitto 
LOCAL_SRC_FILES			:= $(MY_SRC_FIELS)
LOCAL_EXPORT_LDLIBS		:= -lz 
LOCAL_EXPORT_C_INCLUDES	:= $(LOCAL_PATH)/include
include $(PREBUILT_STATIC_LIBRARY)

