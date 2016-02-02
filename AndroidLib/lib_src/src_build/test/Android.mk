LOCAL_PATH				:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE			:= xml-test 
LOCAL_SRC_FILES			:= xml2-test.c 
LOCAL_STATIC_LIBRARIES	:= xml2 
include $(BUILD_EXECUTABLE) 

