ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -Wall -fPIC -I$(top_srcdir)/Platform -I$(top_srcdir)/Platform/Linux/ -I$(top_srcdir)/Include -I$(top_srcdir)/Library/Log -I$(top_srcdir)/Library/SAConfig -I$(top_srcdir)/Library/SAGatherInfo -I$(top_srcdir)/Library/SAClient $(XML_CFLAGS)
bin_PROGRAMS = cagent
cagent_SOURCES = cagent.c cagent.h ../../Platform/Linux/util_path.c ../../Platform/Linux/service.c
cagent_LDFLAGS = $(XML_LIBS) -lpthread -ldl -Wl,-rpath,./ -Wl,-rpath,/usr/local/AgentService/ -release $(PACKAGE_VERSION_MAJOR).$(PACKAGE_VERSION_MINOR).$(PACKAGE_VERSION_PATCH).$(PACKAGE_VERSION_REVISION)
LDADD = -lstdc++ $(top_srcdir)/Library/SAClient/libSAClient.la $(top_srcdir)/Library/SAGatherInfo/libSAGatherInfo.la $(top_srcdir)/Library/SAConfig/libSAConfig.la $(top_srcdir)/Library/Log/libLog.a $(INTLLIBS)
