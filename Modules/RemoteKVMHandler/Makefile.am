AM_CPPFLAGS = -Wall -fPIC -I$(top_srcdir)/Platform/Linux/ -I$(top_srcdir)/Include -I$(top_srcdir)/Library/SAConfig -I$(top_srcdir)/Library/cJSON -I$(top_srcdir)/Library/Log -I$(top_srcdir)/Library/Base64 $(XML_CFLAGS) 
lib_LTLIBRARIES = RemoteKVMHandler.la
RemoteKVMHandler_la_SOURCES = Parser.c kvmconfig.c RemoteKVMHandler.c ../../Platform/Linux/platform.c ../../Platform/Linux/common.c ../../Platform/Linux/network.c
RemoteKVMHandler_la_LDFLAGS = $(XML_LIBS) -L$(top_srcdir)/Library/cJSON -lcJSON -L$(top_srcdir)/Library/Log -lLog -L$(top_srcdir)/Library/Base64 -lBase64 -lpthread -ldl -release $(PACKAGE_VERSION_MAJOR).$(PACKAGE_VERSION_MINOR).$(PACKAGE_VERSION_PATCH).$(PACKAGE_VERSION_REVISION) -module
LDADD = $(top_srcdir)/Library/SAConfig/libSAConfig.la $(INTLLIBS)