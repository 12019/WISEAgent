ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -Wall -fPIC -I$(top_srcdir)/Platform -I$(top_srcdir)/Platform/Linux/ -I$(top_srcdir)/Include -I$(top_srcdir)/Library/Log
bin_PROGRAMS = sawatchdog
sawatchdog_SOURCES = Global.c SAWatchdog.c WatchdogL.c ../../Platform/Linux/service.c ../../Platform/Linux/NamedPipeServer.c
sawatchdog_LDFLAGS = -lpthread -ldl -release $(PACKAGE_VERSION_MAJOR).$(PACKAGE_VERSION_MINOR).$(PACKAGE_VERSION_PATCH).$(PACKAGE_VERSION_REVISION)
LDADD = -lstdc++ $(top_srcdir)/Library/Log/libLog.a $(INTLLIBS)
