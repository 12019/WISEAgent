AM_CPPFLAGS = -Wall -fPIC -I$(top_srcdir)/Platform/Linux/ -I$(top_srcdir)/Include -I$(top_srcdir)/Library/cJSON -I$(top_srcdir)/Library/Log -I$(top_srcdir)/Library/SUSIHelper -I$(top_srcdir)/Library3rdParty/SUSIIOT/include
lib_LTLIBRARIES = SUSIControlHandler.la
SUSIControlHandler_la_SOURCES = Parser.c SUSIControlHandler.cpp ../../Platform/Linux/platform.c ../../Platform/Linux/common.c
SUSIControlHandler_la_LDFLAGS = -L$(top_srcdir)/Library/cJSON -lcJSON -L$(top_srcdir)/Library/Log -lLog -lpthread -ldl -release $(PACKAGE_VERSION_MAJOR).$(PACKAGE_VERSION_MINOR).$(PACKAGE_VERSION_PATCH).$(PACKAGE_VERSION_REVISION) -module
# LDADD = $(top_srcdir)/Library/SAConfig/libSAConfig.la $(INTLLIBS)