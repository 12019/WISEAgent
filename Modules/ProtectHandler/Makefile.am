AM_CPPFLAGS = -Wall -fPIC -I$(top_srcdir)/Platform/Linux/ -I$(top_srcdir)/Include -I$(top_srcdir)/Library/cJSON -I$(top_srcdir)/Library/Log -I$(top_srcdir)/Library/MD5 -I$(top_srcdir)/Library/FtpHelper -I$(top_srcdir)/Library/Base64 -I$(top_srcdir)/Library/DES -D PRTT_LINUX_OS_VERSION="\"$(LINUX_OS)$(OS_VERSION)\""
lib_LTLIBRARIES = ProtectHandler.la
ProtectHandler_la_SOURCES = activate_prtt.c parser_prtt.h public_prtt.h \
			    activate_prtt.h ProtectHandler.c status_prtt.c \
			    capability_prtt.c ProtectHandler.h status_prtt.h \
			    capability_prtt.h ProtectHandlerLog.h unprotect_prtt.c \
			    install_update_prtt.c protect_prtt.c unprotect_prtt.h \
			    install_update_prtt.h protect_prtt.h parser_prtt.c public_prtt.c \
			    ../../Platform/Linux/platform.c ../../Platform/Linux/common.c
ProtectHandler_la_LDFLAGS = -L$(top_srcdir)/Library/cJSON -lcJSON -L$(top_srcdir)/Library/Log -lLog -L$(top_srcdir)/Library/MD5 -lmd5 -L$(top_srcdir)/Library/FtpHelper -lftphelper -L$(top_srcdir)/Library/Base64 -lBase64 -L$(top_srcdir)/Library/DES -lDES -lpthread -ldl -release $(PACKAGE_VERSION_MAJOR).$(PACKAGE_VERSION_MINOR).$(PACKAGE_VERSION_PATCH).$(PACKAGE_VERSION_REVISION) -module
# LDADD = $(top_srcdir)/Library/SAConfig/libSAConfig.la $(INTLLIBS)
