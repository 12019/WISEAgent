AM_CPPFLAGS = -Wall -fPIC -I$(top_srcdir)/Platform/Linux/ -I$(top_srcdir)/Include -I$(top_srcdir)/Library/cJSON -I$(top_srcdir)/Library/Log -I$(top_srcdir)/Library/SAConfig $(XML_CFLAGS)
lib_LTLIBRARIES = SoftwareMonitorHandler.la
SoftwareMonitorHandler_la_SOURCES = Parser.c Parser.h software_config.c software_config.h SoftwareMonitorHandler.c SoftwareMonitorHandler.h SoftwareMonitorLog.h SUSIAgentWatch.h ../../Platform/Linux/platform.c ../../Platform/Linux/common.c
SoftwareMonitorHandler_la_LDFLAGS = $(XML_LIBS) -L$(top_srcdir)/Library/cJSON -lcJSON -L$(top_srcdir)/Library/Log -lLog -lpthread -ldl -release $(PACKAGE_VERSION_MAJOR).$(PACKAGE_VERSION_MINOR).$(PACKAGE_VERSION_PATCH).$(PACKAGE_VERSION_REVISION) -module
LDADD = $(top_srcdir)/Library/SAConfig/libSAConfig.la $(INTLLIBS)
