AM_CPPFLAGS = -Wall -fPIC -I$(top_srcdir)/Platform/Linux/ -I$(top_srcdir)/Library/MD5
bin_PROGRAMS = MD5Test
MD5Test_SOURCES = MD5Test.c ../../Platform/Linux/platform.c
MD5Test_LDFLAGS = -L$(top_srcdir)/Library/MD5 -lmd5 -lpthread -ldl -version-info 1:0:1
LDADD = $(INTLLIBS)
