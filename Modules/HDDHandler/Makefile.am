AM_CPPFLAGS = -Wall -fPIC -I$(top_srcdir)/Platform/Linux/ -I$(top_srcdir)/Include -I$(top_srcdir)/Library/cJSON -I$(top_srcdir)/Library/Log -I$(top_srcdir)/Library/SUSIHelper -I$(top_srcdir)/Library/SQFlashHelper
lib_LTLIBRARIES = HDDHandler.la
HDDHandler_la_SOURCES = Parser.c ThresholdHelper.c HDDHandler.c ../../Platform/Linux/platform.c ../../Platform/Linux/common.c
HDDHandler_la_LDFLAGS = -L$(top_srcdir)/Library/cJSON -lcJSON -L$(top_srcdir)/Library/Log -lLog -L$(top_srcdir)/Library/SUSIHelper -lsusihelper -L$(top_srcdir)/Library/SQFlashHelper -lsqflashhelper -lpthread -ldl -release $(PACKAGE_VERSION_MAJOR).$(PACKAGE_VERSION_MINOR).$(PACKAGE_VERSION_PATCH).$(PACKAGE_VERSION_REVISION) -module
# LDADD = $(top_srcdir)/Library/SAConfig/libSAConfig.la $(INTLLIBS)
