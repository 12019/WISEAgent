AM_CPPFLAGS = -Wall -fPIC -I$(top_srcdir)/Platform/Linux/ -I$(top_srcdir)/Include -I$(top_srcdir)/Library/cJSON -I$(top_srcdir)/Library/Log -I$(top_srcdir)/Library/SAConfig $(XML_CFLAGS)
lib_LTLIBRARIES = ProcessMonitorHandler.la
ProcessMonitorHandler_la_SOURCES = Parser.c process_config.h process_threshold.c \
				   Parser.h ProcessMonitorHandler.c process_threshold.h \
				   process_common.h  ProcessMonitorHandler.h  SUSIAgentWatch.h \
				   process_config.c  ProcessMonitorLog.h \
				   ../../Platform/Linux/platform.c ../../Platform/Linux/common.c
ProcessMonitorHandler_la_LDFLAGS = $(XML_LIBS) -L$(top_srcdir)/Library/cJSON -lcJSON -L$(top_srcdir)/Library/Log -lLog -lpthread -ldl -release $(PACKAGE_VERSION_MAJOR).$(PACKAGE_VERSION_MINOR).$(PACKAGE_VERSION_PATCH).$(PACKAGE_VERSION_REVISION) -module
LDADD = $(top_srcdir)/Library/SAConfig/libSAConfig.la $(INTLLIBS)
