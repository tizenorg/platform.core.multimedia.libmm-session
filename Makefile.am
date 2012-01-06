lib_LTLIBRARIES = libmmfsession.la

includelibmmfsessiondir = $(includedir)/mmf
includelibmmfsession_HEADERS = mm_session.h mm_session_private.h

libmmfsession_la_SOURCES = mm_session.c 

libmmfsession_la_CFLAGS = -I$(srcdir) \
						$(MMCOMMON_CFLAGS) \
						$(AUDIOSESSIONMGR_CFLAGS) \
						$(DLOG_CFLAGS)

libmmfsession_la_LIBADD = $(MMCOMMON_LIBS) \
						$(AUDIOSESSIONMGR_LIBS) \
						$(DLOG_LIBS)
						
libmmfsession_la_LDFLAGS = -Wl,-init, __init_module
libmmfsession_la_LDFLAGS += -Wl,-fini, __fini_module

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = mm-session.pc

