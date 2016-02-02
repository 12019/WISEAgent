include ../../common_config.mk

############################################################################
# Beginming of Developer Modification 
############################################################################
# for building program
application_NAME := 
# for building library
lib_NAME := libadvcarehelper
SOVERSION := 1

program_EXT_OBJS := $(PLATFORM_LINUX_DIR)/util_libloader.o $(PLATFORM_LINUX_DIR)/util_string.o
program_SUBDIRS :=
CFLAGS += -Wall -fPIC
CPPFLAGS += -D_LINUX
LDFLAGS += 
program_INCLUDE_DIRS := $(PLATFORM_LINUX_DIR)
program_LIBRARY_DIRS := 
program_LIBRARIES := $(program_3rdPARTY_LIBRARIES)
############################################################################
# End of Developer Modification 
############################################################################

include ../../common_rule.mk

