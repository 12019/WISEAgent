LOCAL_PATH				:= $(call my-dir)

# define my vars
ifeq ($(NDK_DEBUG), 1)
	MY_SRC_FILES		:= libiconv-debug.a
else
	MY_SRC_FILES		:= libiconv.a
endif


include $(CLEAR_VARS)
LOCAL_MODULE			:= iconv
LOCAL_SRC_FILES			:= $(MY_SRC_FILES)
LOCAL_EXPORT_C_INCLUDES	:= $(LOCAL_PATH)/include
include $(PREBUILT_STATIC_LIBRARY)

