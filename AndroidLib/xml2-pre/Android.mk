LOCAL_PATH				:= $(call my-dir)

# define my vars
ifeq ($(NDK_DEBUG), 1)
	MY_SRC_FILES		:= libxml2-debug.a
else
	MY_SRC_FILES		:= libxml2.a
endif


include $(CLEAR_VARS)
LOCAL_MODULE			:= xml2
LOCAL_SRC_FILES			:= $(MY_SRC_FILES)
LOCAL_STATIC_LIBRARIES	:= iconv
LOCAL_EXPORT_LDLIBS		:= -lz 
LOCAL_EXPORT_C_INCLUDES	:= $(LOCAL_PATH)/include
include $(PREBUILT_STATIC_LIBRARY)

