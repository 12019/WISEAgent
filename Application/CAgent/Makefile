include ../../common_config.mk

############################################################################
# Beginming of Developer Modification 
############################################################################
# for building program
application_NAME := cagent
# for building library
lib_NAME := 
SOVERSION := 

program_EXT_OBJS := $(PLATFORM_LINUX_DIR)/util_path.o $(PLATFORM_LINUX_DIR)/service.o
program_SUBDIRS :=
CFLAGS += -Wall
CPPFLAGS += -D_LINUX -DLOG4Z -I$(ROOTFS)/usr/include/libxml2
LDFLAGS += -Wl,-rpath,./ -Wl,-rpath,/usr/local/AgentService/
program_INCLUDE_DIRS := $(PLATFORMS_DIR) $(PLATFORM_LINUX_DIR) $(application_LIBSDIR)  $(INCLUDE_DIR)
program_LIBRARY_DIRS := $(PROJECT_ROOT_DIR) $(LIB_DIR)/Log $(LIB_DIR)/SAClient $(LIB_DIR)/SAConfig $(LIB_DIR)/SAGatherInfo $(LIB_DIR)/MQTTHelper
program_LIBRARIES :=  SAClient mqtthelper SAConfig SAGatherInfo $(program_3rdPARTY_LIBRARIES) Log pthread dl
############################################################################
# End of Developer Modification 
############################################################################

include ../../common_rule.mk

