AC_PREREQ([2.69])
AC_INIT([glrpp], [0.1.0], [you@example.com])
AC_CONFIG_SRCDIR([include/glrpp])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign subdir-objects no-dist-gzip dist-xz tar-ustar])
AC_LANG([C++])
LT_INIT

# ------------------------------------------------------------
# Compiler checks
# ------------------------------------------------------------
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Require C++20
AX_CHECK_COMPILE_FLAG([-std=c++20], [], [
  AC_MSG_ERROR([C++20 or newer is required])
])

# ------------------------------------------------------------
# Options (matching Meson/CMake options)
# ------------------------------------------------------------

AC_ARG_ENABLE([bindings],
  AS_HELP_STRING([--disable-bindings], [Disable SWIG runtime bindings]),
  [enable_bindings="$enableval"],
  [enable_bindings=yes])

AC_ARG_ENABLE([docs],
  AS_HELP_STRING([--disable-docs], [Disable documentation generation]),
  [enable_docs="$enableval"],
  [enable_docs=yes])

AC_ARG_ENABLE([manpages],
  AS_HELP_STRING([--enable-manpages], [Build manpages]),
  [enable_manpages="$enableval"],
  [enable_manpages=no])

# Package targets
AC_ARG_ENABLE([deb],
  AS_HELP_STRING([--enable-deb], [Enable Debian package target]),
  [enable_deb="$enableval"],
  [enable_deb=no])

AC_ARG_ENABLE([rpm],
  AS_HELP_STRING([--enable-rpm], [Enable RPM package target]),
  [enable_rpm="$enableval"],
  [enable_rpm=no])

AC_ARG_ENABLE([tarball],
  AS_HELP_STRING([--enable-tarball], [Enable tarball package target]),
  [enable_tarball="$enableval"],
  [enable_tarball=no])

AC_ARG_ENABLE([zlib],
  AS_HELP_STRING([--enable-zlib], [Enable zip archive target]),
  [enable_zlib="$enableval"],
  [enable_zlib=no])

# ------------------------------------------------------------
# Python + SWIG checks
# ------------------------------------------------------------

AM_PATH_PYTHON([3.6])

AS_IF([test "x$enable_bindings" = "xyes"], [
  AC_CHECK_PROG([SWIG], [swig], [swig], [no])
  AS_IF([test "x$SWIG" = "xno"], [
    AC_MSG_ERROR([SWIG is required for bindings but was not found.])
  ])
])

# Check for libglr headers + library
AS_IF([test "x$enable_bindings" = "xyes"], [
  AC_CHECK_HEADER([glr.h], [], [
    AC_MSG_ERROR([glr.h not found: required for bindings])
  ])
  AC_CHECK_LIB([glr], [glr_parse], [],
    [AC_MSG_ERROR([libglr not found or missing glr_parse symbol])]
  )
])

AC_CHECK_LIB([dl], [dlopen], [LIBDL="-ldl"], [LIBDL=""])
AC_SUBST([LIBDL])

case $host_os in
  darwin*) glrpp_os_darwin=yes; glrpp_os_windows=no; glrpp_os_unix=yes ;;
  mingw* | msys* | cygwin*) glrpp_os_darwin=no; glrpp_os_windows=yes; glrpp_os_unix=no ;;
  *) glrpp_os_darwin=no; glrpp_os_windows=no; glrpp_os_unix=yes ;;
esac

AM_CONDITIONAL([OS_UNIX], [test "x$glrpp_os_unix" = "xyes"])
AM_CONDITIONAL([OS_DARWIN], [test "x$glrpp_os_darwin" = "xyes"])
AM_CONDITIONAL([OS_WINDOWS], [test "x$glrpp_os_windows" = "xyes"])

# ------------------------------------------------------------
# Output files
# ------------------------------------------------------------

AC_CONFIG_FILES([docs/Doxyfile])

AC_CONFIG_FILES([
  Makefile
])
AC_OUTPUT
