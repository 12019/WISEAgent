AM_CPPFLAGS = -Wall -fPIC -I$(top_srcdir)/Library3rdParty/jpeg-8d1/
bin_PROGRAMS = ScreenshotHelper
ScreenshotHelper_SOURCES = ScreenshotL.c ScreenshotHelperL.c
ScreenshotHelper_LDFLAGS = $(JPEG_LIBS) $(X11_LIBS) -lpthread -ldl -release $(PACKAGE_VERSION_MAJOR).$(PACKAGE_VERSION_MINOR).$(PACKAGE_VERSION_PATCH).$(PACKAGE_VERSION_REVISION)
LDADD = $(top_srcdir)/Library3rdParty/jpeg-8d1/.libs/libjpeg.a $(INTLLIBS)