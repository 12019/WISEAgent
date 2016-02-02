include ../../common_config.mk

############################################################################
# Beginming of Developer Modification 
############################################################################
# for building program
application_NAME := sawatchdog
# for building library
lib_NAME := 
SOVERSION := 

program_EXT_OBJS := $(PLATFORM_LINUX_DIR)/service.o $(PLATFORM_LINUX_DIR)/NamedPipeServer.o
program_SUBDIRS :=
CFLAGS += -Wall
CPPFLAGS += -D_LINUX -DLOG4Z -I$(ROOTFS)/usr/include/libxml2
LDFLAGS += -Wl,-rpath,./ -Wl,-rpath,/usr/local/AgentService/
program_INCLUDE_DIRS := $(PLATFORMS_DIR) $(PLATFORM_LINUX_DIR) $(application_LIBSDIR)  $(INCLUDE_DIR)
program_LIBRARY_DIRS := $(PROJECT_ROOT_DIR) $(LIB_DIR)/Log
program_LIBRARIES := $(program_3rdPARTY_LIBRARIES) Log pthread dl rt
############################################################################
# End of Developer Modification 
############################################################################

include ../../common_rule.mk

