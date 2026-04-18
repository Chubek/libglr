ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = \
    -I$(srcdir)/include \
    -I$(srcdir)/third_party/brigand/include \
    -I$(srcdir)/third_party/compile-time-regular-expressions/include \
    -I$(srcdir)/third_party/frozen/include \
    -I$(srcdir)/third_party/libltdl \
    -I$(srcdir)/third_party/libltdl/libltdl \
    -I$(srcdir)/third_party/magic_enum/include \
    -I$(srcdir)/third_party/nameof/include \
    -I$(srcdir)/third_party/pfr/include \
    -DGLRPP_HEADER_ONLY

EXTRA_DIST = \
    scripts/archive-services.py \
    cmake/glrppConfig.cmake.in \
    cmake/glrppConfigVersion.cmake \
    cmake/glrppTargets.cmake \
    docs/Doxyfile.in \
    docs/CMakeLists.txt \
    docs/meson.build \
    docs/build.ninja \
    docs/Makefile.am \
    docs/mainpage.md \
    docs/doxygen-darkmode.css \
    docs/ltx-header.tex \
    docs/ltx-footer.tex \
    docs/mk-booklet.sh \
    docs/booklet/Manifest.yaml \
    docs/booklet/*.md \
    glrpp.pc

lib_LTLIBRARIES =
noinst_LTLIBRARIES =

if INSTALL_BUNDLED_LIBLTDL
lib_LTLIBRARIES += libltdl.la
else
noinst_LTLIBRARIES += libltdl.la
endif

libltdl_la_SOURCES = \
    third_party/libltdl/ltdl.c \
    third_party/libltdl/lt_dlloader.c \
    third_party/libltdl/lt_error.c \
    third_party/libltdl/lt__alloc.c \
    third_party/libltdl/lt__argz.c \
    third_party/libltdl/lt__dirent.c \
    third_party/libltdl/lt__strl.c \
    third_party/libltdl/slist.c \
    third_party/libltdl/loaders/preopen.c

if OS_UNIX
libltdl_la_SOURCES += third_party/libltdl/loaders/dlopen.c
endif

if OS_DARWIN
libltdl_la_SOURCES += third_party/libltdl/loaders/dyld.c
endif

if OS_WINDOWS
libltdl_la_SOURCES += third_party/libltdl/loaders/loadlibrary.c
endif

libltdl_la_LDFLAGS = -no-undefined -version-info 7:0:0
libltdl_la_LIBADD = $(LIBDL)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = glrpp.pc

install-data-local:
	$(MKDIR_P) $(DESTDIR)$(includedir)
	cp -r $(srcdir)/include/glrpp $(DESTDIR)$(includedir)/
	cp -r $(srcdir)/third_party/brigand/include/brigand $(DESTDIR)$(includedir)/
	cp -r $(srcdir)/third_party/compile-time-regular-expressions/include/ctre $(DESTDIR)$(includedir)/
	cp $(srcdir)/third_party/compile-time-regular-expressions/include/ctre.hpp $(DESTDIR)$(includedir)/
	cp $(srcdir)/third_party/compile-time-regular-expressions/include/ctre-unicode.hpp $(DESTDIR)$(includedir)/
	cp $(srcdir)/third_party/compile-time-regular-expressions/include/unicode-db.hpp $(DESTDIR)$(includedir)/
	cp -r $(srcdir)/third_party/frozen/include/frozen $(DESTDIR)$(includedir)/
	cp -r $(srcdir)/third_party/libltdl/libltdl $(DESTDIR)$(includedir)/
	cp $(srcdir)/third_party/libltdl/ltdl.h $(DESTDIR)$(includedir)/
	cp -r $(srcdir)/third_party/magic_enum/include/magic_enum $(DESTDIR)$(includedir)/
	cp -r $(srcdir)/third_party/nameof/include/nameof $(DESTDIR)$(includedir)/
	cp -r $(srcdir)/third_party/pfr/include/boost $(DESTDIR)$(includedir)/

if ENABLE_DOCS
html:
	$(MAKE) -C docs html

singlehtml:
	$(MAKE) -C docs html

pdf:
	$(MAKE) -C docs pdf

latex:
	$(MAKE) -C docs latex

docs: html singlehtml pdf latex
endif

if ENABLE_DEB
deb:
	$(PYTHON) $(srcdir)/scripts/archive-services.py deb
endif

if ENABLE_RPM
rpm:
	$(PYTHON) $(srcdir)/scripts/archive-services.py rpm
endif

if ENABLE_TARBALL
tarball:
	$(PYTHON) $(srcdir)/scripts/archive-services.py tarball
endif

if ENABLE_ZLIB
zlib:
	$(PYTHON) $(srcdir)/scripts/archive-services.py zlib
endif

.PHONY: html singlehtml pdf latex docs deb rpm tarball zlib
