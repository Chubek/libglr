AC_PREREQ([2.69])
AC_INIT([glrpp], [0.1.0], [you@example.com])
AC_CONFIG_SRCDIR([include/glrpp/config.hpp])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIRS([m4])
AM_INIT_AUTOMAKE([foreign subdir-objects no-dist-gzip dist-xz tar-ustar])
AC_CONFIG_HEADERS([config.h])
AC_LANG([C++])

AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AM_PROG_AR
LT_INIT

saved_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS -std=c++20"
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([[#include <string_view>]], [[std::string_view value{"glrpp"}; return value.empty();]])],
  [],
  [AC_MSG_ERROR([C++20 or newer is required])])

AM_PATH_PYTHON([3.6], [], [:])

AC_ARG_ENABLE([docs],
  AS_HELP_STRING([--disable-docs], [Disable documentation generation]),
  [enable_docs="$enableval"],
  [enable_docs=yes])
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
AC_ARG_ENABLE([install-libltdl],
  AS_HELP_STRING([--disable-install-libltdl], [Do not install the bundled libltdl helper library]),
  [enable_install_libltdl="$enableval"],
  [enable_install_libltdl=yes])

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
AM_CONDITIONAL([ENABLE_DOCS], [test "x$enable_docs" = "xyes"])
AM_CONDITIONAL([ENABLE_DEB], [test "x$enable_deb" = "xyes"])
AM_CONDITIONAL([ENABLE_RPM], [test "x$enable_rpm" = "xyes"])
AM_CONDITIONAL([ENABLE_TARBALL], [test "x$enable_tarball" = "xyes"])
AM_CONDITIONAL([ENABLE_ZLIB], [test "x$enable_zlib" = "xyes"])
AM_CONDITIONAL([INSTALL_BUNDLED_LIBLTDL], [test "x$enable_install_libltdl" = "xyes"])

AC_CONFIG_FILES([
  Makefile
  docs/Doxyfile
  glrpp.pc
])
AC_OUTPUT
