AC_PREREQ(2.61)
AC_INIT(MMSESSION, 1.0, BUG-REPORT-ADDRESS)
AC_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE(mm-session, 1.0)

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

# Checks for libraries.
PKG_CHECK_MODULES(AUDIOSESSIONMGR, audio-session-mgr)
AC_SUBST(AUDIOSESSIONMGR_CFLAGS)
AC_SUBST(AUDIOSESSIONMGR_LIBS)

PKG_CHECK_MODULES(MMCOMMON, mm-common)
AC_SUBST(MMCOMMON_CFLAGS)
AC_SUBST(MMCOMMON_LIBS)

PKG_CHECK_MODULES(DLOG, dlog)
AC_SUBST(DLOG_CFLAGS)
AC_SUBST(DLOG_LIBS)

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([sys/types.h sys/stat.h fcntl.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_MODE_T

# Checks for library functions.
#AC_FUNC_MALLOC
#AC_CHECK_FUNCS([memset])

AC_CONFIG_FILES([Makefile
		 mm-session.pc
		 ])
AC_OUTPUT
