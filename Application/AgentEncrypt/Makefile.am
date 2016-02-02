AM_CPPFLAGS = -Wall -fPIC -I$(top_srcdir)/Platform/Linux/ -I$(top_srcdir)/Library/Log -I$(top_srcdir)/Library/Base64 -I$(top_srcdir)/Library/DES
bin_PROGRAMS = AgentEncrypt
AgentEncrypt_SOURCES = AgentEncrypt.c ../../Platform/Linux/platform.c
AgentEncrypt_LDFLAGS = -L$(top_srcdir)/Library/Log -lLog -L$(top_srcdir)/Library/Base64 -lBase64 -L$(top_srcdir)/Library/DES -lDES -lpthread -ldl -version-info 1:0:1
LDADD = $(INTLLIBS)