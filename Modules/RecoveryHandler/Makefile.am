AM_CPPFLAGS = -Wall -fPIC -I$(top_srcdir)/Platform/Linux/ -I$(top_srcdir)/Include -I$(top_srcdir)/Library/cJSON -I$(top_srcdir)/Library/Log -I$(top_srcdir)/Library/MD5 -I$(top_srcdir)/Library/FtpHelper -I$(top_srcdir)/Library/Base64 -I$(top_srcdir)/Library/DES -I$(top_srcdir)/Library/SQLite -D RCVY_LINUX_OS_VERSION="\"$(LINUX_OS)$(OS_VERSION)\""
lib_LTLIBRARIES = RecoveryHandler.la
RecoveryHandler_la_SOURCES = activate_rcvy.c capability_rcvy.c public_rcvy.c restore_rcvy.h \
			     activate_rcvy.h capability_rcvy.h public_rcvy.h status_rcvy.c \
			     asz_rcvy.c install_update_rcvy.c RecoveryHandler.c status_rcvy.h \
			     asz_rcvy.h install_update_rcvy.h RecoveryHandler.h \
			     backup_rcvy.c parser_rcvy.c RecoveryLog.h \
			     backup_rcvy.h parser_rcvy.h restore_rcvy.c \
			     ../../Platform/Linux/platform.c ../../Platform/Linux/common.c
RecoveryHandler_la_LDFLAGS = -L$(top_srcdir)/Library/cJSON -lcJSON -L$(top_srcdir)/Library/Log -lLog -L$(top_srcdir)/Library/MD5 -lmd5 -L$(top_srcdir)/Library/FtpHelper -lftphelper -L$(top_srcdir)/Library/Base64 -lBase64 -L$(top_srcdir)/Library/DES -lDES -L$(top_srcdir)/Library/SQLite -lsqlite3 -lpthread -ldl -release $(PACKAGE_VERSION_MAJOR).$(PACKAGE_VERSION_MINOR).$(PACKAGE_VERSION_PATCH).$(PACKAGE_VERSION_REVISION) -module
# LDADD = $(top_srcdir)/Library/SAConfig/libSAConfig.la $(INTLLIBS)
