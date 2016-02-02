include ../../common_config.mk
include ../../common_version.mk

############################################################################
# Beginming of Developer Modification 
############################################################################
# for building program
application_NAME := GatherInfoTest
# for building library
lib_NAME := 
SOVERSION := 
XML2_INC_DIR := $(ROOTFS)/usr/include/libxml2
program_EXT_OBJS := 
program_SUBDIRS :=
CFLAGS += -Wall
CPPFLAGS += -D_LINUX
LDFLAGS += -Wl,-rpath,./
program_INCLUDE_DIRS := $(PLATFORM_LINUX_DIR) $(application_LIBSDIR)  $(INCLUDE_DIR) $(XML2_INC_DIR)
program_LIBRARY_DIRS := $(PROJECT_ROOT_DIR) $(LIB_DIR)/Log $(LIB_DIR)/SAGatherInfo
program_LIBRARIES :=  xml2 SAGatherInfo Log pthread dl rt
############################################################################
# End of Developer Modification 
############################################################################

include ../../common_rule.mk

