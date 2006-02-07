dnl Initialise autoconf

AC_INIT([gridframe],[1.0.0],[alriddoch@zepler.org])
AC_CONFIG_SRCDIR([src/framework.cpp])

AC_CANONICAL_TARGET

dnl Initialise automake

AM_INIT_AUTOMAKE

dnl Test for C++ compiler

AC_PROG_CXX

CFLAGS="$CFLAGS -Wall"
CXXFLAGS="$CXXFLAGS -Wall"

AC_LANG(C++)

dnl Test for libraries

SDL_VERSION=1.2.0
AM_PATH_SDL($SDL_VERSION)
CPPFLAGS="$CPPFLAGS $SDL_CFLAGS"
LIBS="$LIBS $SDL_LIBS -L/usr/X11R6/lib"

AC_CHECK_LIB(SDL_image,IMG_Load)
AC_CHECK_LIB(GL,glViewport)
AC_CHECK_LIB(GLU,gluPerspective)
AC_CHECK_LIB(X11,main)
AC_CHECK_LIB(Xi,main)
AC_CHECK_LIB(Xmu,main)

dnl Generate files
AC_CONFIG_FILES([
	Makefile
	src/Makefile
])
AC_OUTPUT
