AM_CPPFLAGS = -Wall -fPIC -I$(top_srcdir)/Platform/Linux/ -I$(top_srcdir)/Include -I$(top_srcdir)/Library/AdvCareHelper -I$(top_srcdir)/Library/SUSIHelper $(XML_CFLAGS)
lib_LTLIBRARIES = libSAGatherInfo.la
libSAGatherInfo_la_SOURCES = SAGatherInfo.c ../../Platform/Linux/util_os.c ../../Platform/Linux/network.c
libSAGatherInfo_la_LDFLAGS = $(XML_LIBS) -L$(top_srcdir)/Library/SUSIHelper/ -lsusihelper -L$(top_srcdir)/Library/AdvCareHelper/ -ladvcarehelper -lpthread -ldl -release $(PACKAGE_VERSION_MAJOR).$(PACKAGE_VERSION_MINOR).$(PACKAGE_VERSION_PATCH).$(PACKAGE_VERSION_REVISION)
include_HEADERS = $(top_srcdir)/Library/SAGatherInfo/SAGatherInfo.h
# LDADD = $(top_srcdir)/Library/SUSIHelper/libsusihelper.a $(top_srcdir)/Library/AdvCareHelper/libadvcarehelper.a $(INTLLIBS) 
