#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])

m4_define([VERSION_MAJOR], 
	m4_esyscmd([cut -d'.' -f1 VERSION | tr -d '\n']))
m4_define([VERSION_MINOR], 
	m4_esyscmd([cut -d'.' -f2 VERSION | tr -d '\n']))
m4_define([VERSION_PATCH], 
	m4_esyscmd([cut -d'.' -f3 VERSION | tr -d '\n']))
m4_define([VERSION_REVISION], 
	m4_esyscmd([cut -d'.' -f4 VERSION | tr -d '\n']))

AC_INIT([CAgentIoT], [VERSION])
AM_INIT_AUTOMAKE([foreign -Wall -Werror subdir-objects])
AC_CONFIG_SRCDIR([Include/susiaccess_def.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
#AM_PROG_AR
LT_INIT

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AM_PROG_CC_C_O
AC_PROG_LIBTOOL


# Check STDLIB
AC_ARG_WITH(
	[stdlib],
	[AS_HELP_STRING([--with-stdlib=LIB],[specify the C++ standard library to use [default=system]])],
	[],[with_stdlib=system]
)
AS_IF([test "x$with_stdlib" != "xsystem"],
	[CXXFLAGS="$CXXFLAGS -stdlib=$with_stdlib"
	 LDFLAGS="$LDFLAGS -stdlib=$with_stdlib"]
)

# Checks for libraries.
#### Find OpenSSL
AC_MSG_CHECKING([for --with-openssl])
AC_ARG_WITH(
	[openssl],
	[AS_HELP_STRING([--with-openssl],[/absolute/path/to/openssl-base])],
	[ LDFLAGS="$LDFLAGS -L$with_openssl/lib";
	  CFLAGS="$CFLAGS -I$with_openssl/include/";
	  LIBS="-lssl";
	  AC_MSG_RESULT([$with_openssl])
	],
	[AC_MSG_RESULT([no])]
)

AC_CHECK_LIB([crypto],[CRYPTO_new_ex_data], [], [AC_MSG_ERROR([OpenSSL libraries required])])
AC_CHECK_LIB([ssl],[SSL_library_init], [], [AC_MSG_ERROR([OpenSSL libraries required])])

AC_CHECK_HEADERS([openssl/crypto.h openssl/x509.h openssl/pem.h openssl/ssl.h openssl/err.h],[],[AC_MSG_ERROR([OpenSSL headers required])])

#### Check for libcurl
LIBCURL_CHECK_CONFIG([yes],[],[],[AC_MSG_ERROR([libcurl development files required])])
PKG_CHECK_MODULES(CURL, libcurl >= 7.19.7)

# Checks for header files.
AC_CHECK_HEADERS([netdb.h stddef.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([gethostbyname memset socket strcasecmp strchr strncasecmp strstr])

# Checks for XML2 
PKG_CHECK_MODULES(XML, libxml-2.0 >= 2.4)

# Checks for X11 
PKG_CHECK_MODULES(X11, x11,,[AC_MSG_WARN("x11 missing psensor will not be built")])
AM_CONDITIONAL(X11, test -n "$X11_LIBS")
AC_SUBST(X11_CFLAGS)
AC_SUBST(X11_LIBS)

# Checks for xtst
PKG_CHECK_MODULES([XTST], [xtst >= 1.1.0])
AC_SUBST(XTST_LIBS)

# Checks for header files.
AC_PATH_X
AC_FUNC_ALLOCA
AC_CHECK_HEADERS([OS.h arpa/inet.h fcntl.h float.h inttypes.h limits.h mach/mach.h malloc.h memory.h netdb.h netinet/in.h sgtty.h stddef.h stdint.h stdlib.h string.h strings.h sys/file.h sys/ioctl.h sys/mount.h sys/param.h sys/socket.h sys/statvfs.h sys/systeminfo.h sys/time.h sys/timeb.h syslog.h termio.h termios.h unistd.h utime.h wchar.h])

# Checks for typedefs, structures, and compiler characteristics.
#AC_CHECK_HEADER_STDBOOL
AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_CHECK_MEMBERS([struct stat.st_blksize])
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_REALLOC
AC_FUNC_STRERROR_R
AC_CHECK_FUNCS([alarm clock_gettime dup2 fdatasync floor ftime ftruncate gethostbyaddr gethostbyname gethostname gethrtime getpagesize gettimeofday inet_ntoa isascii localtime_r memchr memmove memset mkdir pow select socket strcasecmp strchr strdup strerror strncasecmp strndup strpbrk strrchr strspn strstr strtol strtoul sysinfo uname utime])

AC_CONFIG_FILES([Makefile
		 Library/Log/Makefile
		 Library/cJSON/Makefile
		 Library/DES/Makefile
		 Library/Base64/Makefile
		 Library/MQTTHelper/Makefile
		 Library/SAClient/Makefile
		 Library/SAConfig/Makefile
		 Library/AdvCareHelper/Makefile
		 Library/SUSIHelper/Makefile
		 Library/FtpHelper/Makefile
		 Library/MD5/Makefile
		 Library/SQLite/Makefile
		 Library/SQFlashHelper/Makefile
		 Library3rdParty/Smart/smartcheck/Makefile
		 Library/SAGatherInfo/Makefile
		 Library/SAManager/Makefile
		 Library/SAHandlerLoader/Makefile
		 Library/SAGeneralHandler/Makefile
		 Application/AgentEncrypt/Makefile
		 Application/ScreenshotHelperL/Makefile
		 Application/CAgent/Makefile
		 Application/PlatformCheck/Makefile
		 Modules/TerminalHandler/Makefile
		 Modules/ScreenshotHandler/Makefile
		 Modules/RemoteKVMHandler/Makefile
		 Modules/SoftwareMonitorHandler/Makefile
		 Modules/ProcessMonitorHandler/Makefile
		 Modules/PowerOnOffHandler/Makefile
		 Modules/ProtectHandler/Makefile
		 Modules/RecoveryHandler/Makefile
		 Modules/HDDHandler/Makefile
		 Modules/MonitoringHandler/Makefile
		 Modules/NetMonitorHandler/Makefile
		 Modules/SUSIControlHandler/Makefile
		 Modules/IoTGWHandler/Makefile
		 Application/WatchdogL/Makefile
		 Library/MessageGenerator/Makefile
		 Sample/HandlerSample/Makefile
		 Sample/SampleAgent/Makefile])
AC_SUBST([PACKAGE_VERSION_MAJOR],[VERSION_MAJOR])
AC_SUBST([PACKAGE_VERSION_MINOR],[VERSION_MINOR])
AC_SUBST([PACKAGE_VERSION_PATCH],[VERSION_PATCH])
AC_SUBST([PACKAGE_VERSION_REVISION],[VERSION_REVISION])
AC_OUTPUT
