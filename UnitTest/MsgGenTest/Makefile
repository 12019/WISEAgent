include ../../common_config.mk
include ../../common_version.mk

############################################################################
# Beginming of Developer Modification 
############################################################################
# for building program
application_NAME := MsgGenTest
# for building library
lib_NAME := 
SOVERSION := 

CJSON_INC_DIR :=$(LIB_DIR)/cJSON

program_EXT_OBJS := $(PLATFORM_LINUX_DIR)/platform.o
program_SUBDIRS :=
CFLAGS += -Wall
CPPFLAGS += -D_LINUX
LDFLAGS += -Wl,-rpath,./
program_INCLUDE_DIRS := $(PLATFORM_LINUX_DIR) $(application_LIBSDIR)  $(INCLUDE_DIR) $(LIB_DIR)/MessageGenerater $(CJSON_INC_DIR)
program_LIBRARY_DIRS := $(PROJECT_ROOT_DIR) $(LIB_DIR)/MessageGenerater $(CJSON_INC_DIR)
program_LIBRARIES :=  msggen cJSON pthread dl rt
############################################################################
# End of Developer Modification 
############################################################################

include ../../common_rule.mk

