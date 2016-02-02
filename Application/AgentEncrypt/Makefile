include ../../common_config.mk

############################################################################
# Beginming of Developer Modification 
############################################################################
# for building program
application_NAME := AgentEncrypt
# for building library
lib_NAME := 
SOVERSION := 

program_EXT_OBJS := $(PLATFORM_LINUX_DIR)/platform.o
program_SUBDIRS :=
CFLAGS += -Wall -fPIC
CPPFLAGS += -D_LINUX -DENC_ONLY -fPIC
LDFLAGS += -Wl,-rpath,./
program_INCLUDE_DIRS := $(PLATFORM_LINUX_DIR) $(application_LIBSDIR)  $(INCLUDE_DIR) $(LIB_DIR)/Log $(LIB_DIR)/DES $(LIB_DIR)/Base64
program_LIBRARY_DIRS := $(PROJECT_ROOT_DIR) $(LIB_DIR)/Log $(LIB_DIR)/Base64 $(LIB_DIR)/DES 
program_LIBRARIES :=  Base64 DES $(program_3rdPARTY_LIBRARIES) Log dl
############################################################################
# End of Developer Modification 
############################################################################

include ../../common_rule.mk



