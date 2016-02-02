AM_CPPFLAGS = -Wall -fPIC -I$(top_srcdir)/Platform/Linux/ -I$(top_srcdir)/Include -I$(top_srcdir)/Library/cJSON -I$(top_srcdir)/Library/Log -I$(top_srcdir)/Library/Base64 -I$(top_srcdir)/Library/DES
lib_LTLIBRARIES = ServerRedundancyHandler.la
ServerRedundancyHandler_la_SOURCES = Parser.c ServerRedundancyHandler.c ../../Platform/Linux/platform.c ../../Platform/Linux/common.c
ServerRedundancyHandler_la_LDFLAGS = -L$(top_srcdir)/Library/cJSON -lcJSON -L$(top_srcdir)/Library/Log -lLog -L$(top_srcdir)/Library/Base64 -lBase64 -L$(top_srcdir)/Library/DES -lDES -lpthread -ldl -release $(PACKAGE_VERSION_MAJOR).$(PACKAGE_VERSION_MINOR).$(PACKAGE_VERSION_PATCH).$(PACKAGE_VERSION_REVISION) -module
