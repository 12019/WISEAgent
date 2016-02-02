EXTRA_DIST=tightvnc-1.3dev5-vncviewer-alpha-cursor.patch README.LibVNCServer
CFLAGS=-g -Wall

SUBDIRS=libvncserver libvncclient x11vnc classes
DIST_SUBDIRS=libvncserver libvncclient x11vnc classes


includedir=$(prefix)/include/rfb
#noinst_HEADERS=rfb.h rfbconfig.h rfbint.h rfbproto.h keysym.h rfbregion.h

noinst_HEADERS=rfb/rfb.h rfb/rfbconfig.h rfb/rfbint.h rfb/rfbproto.h \
	rfb/keysym.h rfb/rfbregion.h rfb/rfbclient.h

$(PACKAGE)-$(VERSION).tar.gz: dist

if HAVE_RPM
# Rule to build RPM distribution package
rpm: $(PACKAGE)-$(VERSION).tar.gz $(PACKAGE).spec
	cp $(PACKAGE)-$(VERSION).tar.gz @RPMSOURCEDIR@
	rpmbuild -ba $(PACKAGE).spec
endif

t:
	$(MAKE) -C test test

