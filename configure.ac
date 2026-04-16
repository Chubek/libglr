AC_INIT([libglr], [1.0.0], [bugs@example.invalid])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AC_CONFIG_SRCDIR([src/glr/grammar.c])
AC_CONFIG_HEADERS([config.h])
AC_PROG_CC
AM_PROG_AR
LT_INIT

AC_ARG_ENABLE([sharedlib],
  [AS_HELP_STRING([--enable-sharedlib], [build libglr as a shared library @<:@default=no@:>@])],
  [build_sharedlib="$enableval"],
  [build_sharedlib=no])
AM_CONDITIONAL([BUILD_SHAREDLIB], [test "x$build_sharedlib" = "xyes"])

AC_ARG_ENABLE([install-local],
  [AS_HELP_STRING([--enable-install-local], [install under ~/.local instead of /usr/local])],
  [install_local="$enableval"],
  [install_local=no])

AS_IF([test "x$install_local" = "xyes"], [
  AS_IF([test -z "$HOME"], [
    AC_MSG_ERROR([--enable-install-local requires HOME to be set])
  ])
  local_prefix="$HOME/.local"
  AC_SUBST([prefix], ["$local_prefix"])
  AC_SUBST([exec_prefix], ['${prefix}'])
  AC_SUBST([libdir], ['${exec_prefix}/lib'])
  AC_SUBST([includedir], ['${prefix}/include'])
  AC_SUBST([datadir], ['${prefix}/share'])
])

AC_CONFIG_FILES([
 Makefile
 tests/Makefile
 disambstd/Makefile
 rewritelib/Makefile
 bindings/Makefile
])
AC_OUTPUT
