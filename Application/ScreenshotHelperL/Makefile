include ../../common_config.mk

############################################################################
# Beginming of Developer Modification 
############################################################################
# for building program
application_NAME := ScreenshotHelper
# for building library
lib_NAME := 
SOVERSION := 
LIB_JPEG = jpeg
LIB_X11 = X11
CFLAGS += -Wall
CPPFLAGS += -D_LINUX
LDFLAGS += -Wl,-rpath,./
program_INCLUDE_DIRS := .  ${LIB_3RD_DIR}/jpeg-8d1
program_LIBRARY_DIRS := .  ${LIB_3RD_DIR}/jpeg-8d1/.libs
program_LIBRARIES :=  $(LIB_JPEG) $(LIB_X11)
############################################################################
# End of Developer Modification 
############################################################################

include ../../common_rule.mk



