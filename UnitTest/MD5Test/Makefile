include ../../common_config.mk

############################################################################
# Beginming of Developer Modification 
############################################################################
# for building program
application_NAME := MD5Test
# for building library
lib_NAME := 
SOVERSION := 

program_EXT_OBJS := $(PLATFORM_LINUX_DIR)/platform.o
MD5_INC_DIR := $(LIB_DIR)/MD5

program_SUBDIRS :=
CFLAGS += -Wall -fPIC
CPPFLAGS += -D_LINUX
LDFLAGS += -Wl,-rpath,./
program_INCLUDE_DIRS := $(PLATFORM_LINUX_DIR) $(application_LIBSDIR)  $(INCLUDE_DIR) $(MD5_INC_DIR)
program_LIBRARY_DIRS := $(PROJECT_ROOT_DIR) $(MD5_INC_DIR)
program_LIBRARIES :=  md5 $(program_3rdPARTY_LIBRARIES) pthread dl
############################################################################
# End of Developer Modification 
############################################################################

include ../../common_rule.mk



