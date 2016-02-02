LOCAL_PATH				:= $(call my-dir)

include $(LOCAL_PATH)/config_android.mk 

include $(CLEAR_VARS)

MY_C_FILE_LIST			:= $(wildcard $(LOCAL_PATH)/lib/*.c)

# Module
LOCAL_MODULE 			:= mosquitto
LOCAL_CFLAGS 			:= $(LIB_CFLAGS) 
#LOCAL_LDFLAGS 			:= -lz -ldl
LOCAL_C_INCLUDES		:= $(LOCAL_PATH)/lib
#LOCAL_STATIC_LIBRARIES	:= openssl
LOCAL_SRC_FILES			:= $(MY_C_FILE_LIST:$(LOCAL_PATH)/%=%)

# Export and Build 
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/lib 
#include $(BUILD_SHARED_LIBRARY)
include $(BUILD_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE 	:= mosquitto_sub
LOCAL_CFLAGS 	:= $(CLIENT_CFLAGS)
LOCAL_C_INCLUDES:= $(LOCAL_PATH)/lib
#LOCAL_SHARED_LIBRARIES := mosquitto
LOCAL_STATIC_LIBRARIES := mosquitto
LOCAL_SRC_FILES	:= client/sub_client.c
LOCAL_SRC_FILES	+= client/client_shared.c 
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE 	:= mosquitto_pub
LOCAL_CFLAGS 	:= $(CLIENT_CFLAGS)
LOCAL_C_INCLUDES:= $(LOCAL_PATH)/lib
#LOCAL_SHARED_LIBRARIES := mosquitto
LOCAL_STATIC_LIBRARIES := mosquitto
LOCAL_SRC_FILES	:= client/pub_client.c 
LOCAL_SRC_FILES	+= client/client_shared.c 
include $(BUILD_EXECUTABLE)


