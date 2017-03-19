AC_PREREQ([2.69])
AC_INIT([tumblr-dl], [0.1], [https://github.com/dx7/tumblr-dl/issues])
AC_CONFIG_SRCDIR([src/tumblr-dl.c])

## Checks for programs.
AC_LANG(C)
AC_PROG_CC

## Checks for libraries.
m4_include([m4/libcurl.m4])
LIBCURL_CHECK_CONFIG([yes])

## Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h])

## Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

## Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([regcomp])

AC_OUTPUT([Makefile])
