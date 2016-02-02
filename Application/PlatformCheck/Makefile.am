AM_CPPFLAGS = -Wall -fPIC -I$(top_srcdir)/Platform/Linux/ -I$(top_srcdir)/Library/AdvCareHelper -I$(top_srcdir)/Library/SUSIHelper $(XML_CFLAGS)
bin_PROGRAMS = PlatformCheck
PlatformCheck_SOURCES = PlatformCheck.c
PlatformCheck_LDFLAGS = $(XML_LIBS) -L$(top_srcdir)/Library/SUSIHelper/ -lsusihelper -L$(top_srcdir)/Library/AdvCareHelper/ -ladvcarehelper -ldl -release $(PACKAGE_VERSION_MAJOR).$(PACKAGE_VERSION_MINOR).$(PACKAGE_VERSION_PATCH).$(PACKAGE_VERSION_REVISION)
LDADD = $(INTLLIBS)
