
AC_PREREQ([2.67])
AC_COPYRIGHT([Copyright (c) 2011 Osamu MATSUMOTO])
AC_INIT([xtimepro], [0.9], [osamu@users.sourceforge.jp])
AM_INIT_AUTOMAKE([1.9 foreign])
AC_PROG_CC
AC_PATH_X
AC_CONFIG_SRCDIR([xtimepro.c])
AC_CONFIG_HEADERS([config.h])
AC_CHECK_LIB(X11, [main], , AC_MSG_ERROR(Library X11 not found. cf. Try install libx11-dev.))
AC_CHECK_LIB(Xext,[main], , AC_MSG_ERROR(Library Xext not found. cf. Try install libxext-dev.))
AC_CHECK_LIB(Xss, [main], , AC_MSG_ERROR(Library Xss not found. cf. Try install libxss-dev.))
AC_CHECK_HEADERS( stdlib.h string.h unistd.h X11/Xlib.h X11/Xutil.h X11/extensions/scrnsaver.h )
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
