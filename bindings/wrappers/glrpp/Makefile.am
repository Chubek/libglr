# ============================================================
# Basic variables
# ============================================================

ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = \
    -I$(srcdir)/include \
    -I$(srcdir)/third_party/brigand/include \
    -I$(srcdir)/third_party/ctre/include \
    -I$(srcdir)/third_party/frozen/include \
    -I$(srcdir)/third_party/magic_enum/include \
    -I$(srcdir)/third_party/nameof/include \
    -I$(srcdir)/third_party/pfr/include \
    -DGLRPP_HEADER_ONLY

EXTRA_DIST = \
    glrpp/scripts/archive-services.py \
    glrpp/scripts/docstring-services.py \
    cmake/glrppConfig.cmake \
    cmake/glrppConfigVersion.cmake \
    cmake/glrppTargets.cmake \
    bindings/glr.i

# ============================================================
# Header-only library (phony)
# ============================================================

noinst_LIBRARIES =
noinst_PROGRAMS =
noinst_DATA =

glrpp:
	@echo "Header-only library: nothing to build."


# ============================================================
# SWIG bindings (optional)
# ============================================================

if ENABLE_BINDINGS

SWIG_SOURCES = glrpp_glr_bindings.cpp glrpp_glr_bindings.h

$(SWIG_SOURCES): bindings/glr.i
	$(SWIG) -c++ -python -o glrpp_glr_bindings.cpp $(srcdir)/bindings/glr.i

# Shared library
lib_LTLIBRARIES = libglrpp_glr_bindings.la
libglrpp_glr_bindings_la_SOURCES = glrpp_glr_bindings.cpp
libglrpp_glr_bindings_la_LDFLAGS = -shared
libglrpp_glr_bindings_la_LIBADD = -lglr

endif


# ============================================================
# Installation rules
# ============================================================

# Install public headers
install-data-local:
	mkdir -p $(DESTDIR)$(includedir)
	cp -r $(srcdir)/include/glrpp $(DESTDIR)$(includedir)/

# Install bundled third_party
install-exec-local:
	mkdir -p $(DESTDIR)$(includedir)/glrpp
	cp -r $(srcdir)/third_party $(DESTDIR)$(includedir)/glrpp/

# Install CMake package metadata
install-cmake:
	mkdir -p $(DESTDIR)$(libdir)/cmake/glrpp
	cp cmake/glrppConfig*.cmake cmake/glrppTargets.cmake \
		$(DESTDIR)$(libdir)/cmake/glrpp/


# ============================================================
# Documentation (via Python script)
# ============================================================

if ENABLE_DOCS
html:
	$(PYTHON) $(srcdir)/glrpp/scripts/docstring-services.py html

singlehtml:
	$(PYTHON) $(srcdir)/glrpp/scripts/docstring-services.py singlehtml

pdf:
	$(PYTHON) $(srcdir)/glrpp/scripts/docstring-services.py pdf

latex:
	$(PYTHON) $(srcdir)/glrpp/scripts/docstring-services.py latex

install-docs:
	$(PYTHON) $(srcdir)/glrpp/scripts/docstring-services.py install

docs: html singlehtml pdf latex install-docs
endif


# ============================================================
# Documentation bundling via env var GLRPP_BUNDLE_DOCS
# ============================================================

bundle-docs:
	$(PYTHON) $(srcdir)/glrpp/scripts/archive-services.py bundle $(GLRPP_BUNDLE_DOCS)


# ============================================================
# Packaging targets
# ============================================================

if ENABLE_DEB
deb:
	$(PYTHON) $(srcdir)/glrpp/scripts/archive-services.py deb
endif

if ENABLE_RPM
rpm:
	$(PYTHON) $(srcdir)/glrpp/scripts/archive-services.py rpm
endif

if ENABLE_TARBALL
tarball:
	$(PYTHON) $(srcdir)/glrpp/scripts/archive-services.py tarball
endif

if ENABLE_ZLIB
zlib:
	$(PYTHON) $(srcdir)/glrpp/scripts/archive-services.py zlib
endif


# ============================================================
# Manpages
# ============================================================

if ENABLE_MANPAGES

NUROFF = nuroff

build-man:
	$(NUROFF) $(srcdir)/glrpp/man

install-man:
	mkdir -p $(DESTDIR)$(mandir)
	cp -r $(srcdir)/glrpp/man $(DESTDIR)$(mandir)/

endif


# ============================================================
# Phony targets
# ============================================================

.PHONY: glrpp html pdf latex singlehtml docs deb rpm tarball zlib \
        install-cmake build-man install-man bundle-docs
