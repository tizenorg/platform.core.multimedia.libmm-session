lib_LTLIBRARIES = libmmfsession.la

includelibmmfsessiondir = $(includedir)/mmf
includelibmmfsession_HEADERS = mm_session.h mm_session_private.h

libmmfsession_la_SOURCES = mm_session.c

libmmfsession_la_CFLAGS = -I$(srcdir) \
						$(MMCOMMON_CFLAGS)

libmmfsession_la_LIBADD = $(MMCOMMON_LIBS)

libmmfsession_la_LDFLAGS = -Wl,-init, __init_module
libmmfsession_la_LDFLAGS += -Wl,-fini, __fini_module

libmmfsession_la_CFLAGS += $(MMLOG_CFLAGS) -DMMF_LOG_OWNER=0x00100000 -DMMF_DEBUG_PREFIX=\"MMF-SESSION\"
libmmfsession_la_LIBADD += $(MMLOG_LIBS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = mm-session.pc

