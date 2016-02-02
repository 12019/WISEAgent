LOCAL_PATH				:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE			:= iconv
LOCAL_CFLAGS			:= \
	-Wno-multichar \
	-DANDROID \
	-DLIBDIR="c" \
	-DBUILDING_LIBICONV \
	-DIN_LIBRARY

LOCAL_SRC_FILES			:= \
	libcharset/lib/localcharset.c \
	lib/iconv.c \
	lib/relocatable.c

LOCAL_C_INCLUDES		:= \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/libcharset \
	$(LOCAL_PATH)/lib \
	$(LOCAL_PATH)/libcharset/include \
	$(LOCAL_PATH)/srclib

LOCAL_EXPORT_C_INCLUDES	:= $(LOCAL_C_INCLUDES)
include $(BUILD_STATIC_LIBRARY)


# Test 
#include $(CLEAR_VARS)  
#LOCAL_MODULE 			:= genutf8    
#LOCAL_SRC_FILES 		:= tests/genutf8.c  
#LOCAL_STATIC_LIBRARIES	:= iconv  
#include $(BUILD_EXECUTABLE) 

