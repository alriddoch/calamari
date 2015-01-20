dnl Initialise autoconf

AC_INIT([calamari], [1.0.0], [alriddoch@googlemail.com])
AC_CONFIG_SRCDIR([src/calamari.cpp])

AC_CANONICAL_TARGET

dnl Initialise automake

AM_INIT_AUTOMAKE

dnl Test for C compiler

AC_PROG_CXX

CXXFLAGS="$CXXFLAGS -Wall -fno-math-errno"

AC_LANG(C++)

dnl Test for libraries

SDL_VERSION=2.0.0
AM_PATH_SDL2($SDL_VERSION)
CPPFLAGS="$CPPFLAGS $SDL_CFLAGS"
LIBS="$LIBS $SDL_LIBS"

AX_CXX_COMPILE_STDCXX_11

AC_CHECK_LIB(GL,glViewport)
AC_CHECK_LIB(GLEW,glewInit)

dnl Generate files
AC_CONFIG_FILES([
	Makefile
	src/Makefile
])
AC_OUTPUT
