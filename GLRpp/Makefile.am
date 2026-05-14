# Makefile.am for GLRpp

ACLOCAL_AMFLAGS = -I m4

# Header-only library - no compilation needed
nobase_include_HEADERS = \
	GLRpp/GLRpp.hpp \
	GLRpp/Polyfills.hpp \
	GLRpp/DSLUtils.hpp \
	GLRpp/rewrite/GrammarIR.hpp \
	GLRpp/rewrite/RewritePass.hpp \
	GLRpp/rewrite/RewritePipeline.hpp \
	GLRpp/rewrite/native/NativePasses.hpp \
	GLRpp/rewrite/equinox/EquinoxPasses.hpp \
	GLRpp/rewrite/syntax/SyntaxDSL.hpp

# pkg-config file
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = glrpp.pc

# Doxygen documentation
if HAVE_DOXYGEN
doxyfile.stamp: Doxyfile
	$(DOXYGEN) Doxyfile
	echo Timestamp > doxyfile.stamp

CLEANFILES = doxyfile.stamp

all-local: doxyfile.stamp

clean-local:
	rm -rf $(top_builddir)/docs/html
endif

# Extra files to distribute
EXTRA_DIST = \
	Doxyfile.in \
	README.md \
	LICENSE \
	cmake/glrpp.pc.in

# Subdirectories (if you add examples or tests later)
# SUBDIRS = examples tests
