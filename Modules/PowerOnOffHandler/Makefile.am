AM_CPPFLAGS = -Wall -fPIC -I$(top_srcdir)/Platform/Linux/ -I$(top_srcdir)/Include -I$(top_srcdir)/Library/SAConfig -I$(top_srcdir)/Library/cJSON -I$(top_srcdir)/Library/Log $(XML_CFLAGS) 
lib_LTLIBRARIES = PowerOnOffHandler.la
PowerOnOffHandler_la_SOURCES = amtconfig.c Parser.c PowerOnOffHandler.c ../../Platform/Linux/platform.c ../../Platform/Linux/common.c ../../Platform/Linux/network.c
PowerOnOffHandler_la_LDFLAGS = $(XML_LIBS) -L$(top_srcdir)/Library/cJSON -lcJSON -L$(top_srcdir)/Library/Log -lLog -lpthread -ldl -release $(PACKAGE_VERSION_MAJOR).$(PACKAGE_VERSION_MINOR).$(PACKAGE_VERSION_PATCH).$(PACKAGE_VERSION_REVISION) -module
LDADD = $(top_srcdir)/Library/SAConfig/libSAConfig.la $(INTLLIBS)