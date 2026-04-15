SUBDIRS = tests disambstd rewritelib bindings

AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/third_party/sfsexp/src

noinst_LIBRARIES = libsfsexp_local.a
lib_LTLIBRARIES = libglr.la

libsfsexp_local_a_SOURCES = \
  third_party/sfsexp/src/cstring.c \
  third_party/sfsexp/src/event_temp.c \
  third_party/sfsexp/src/faststack.c \
  third_party/sfsexp/src/io.c \
  third_party/sfsexp/src/parser.c \
  third_party/sfsexp/src/sexp.c \
  third_party/sfsexp/src/sexp_memory.c \
  third_party/sfsexp/src/sexp_ops.c \
  third_party/sfsexp/src/sexp_vis.c

libglr_la_SOURCES = \
  src/glr/disambiguate.c \
  src/glr/forest.c \
  src/glr/fork.c \
  src/glr/grammar.c \
  src/glr/graph.c \
  src/glr/parser.c \
  src/glr/reduction.c \
  src/glr/rewrite.c \
  src/glr/stack.c
libglr_la_LIBADD = libsfsexp_local.a
libglr_la_LDFLAGS = -version-info 1:0:0

pkgincludedir = $(includedir)/glr
pkginclude_HEADERS = \
  include/glr/disambiguate.h \
  include/glr/forest.h \
  include/glr/fork.h \
  include/glr/glr.h \
  include/glr/grammar.h \
  include/glr/graph.h \
  include/glr/parser.h \
  include/glr/reduction.h \
  include/glr/rewrite.h \
  include/glr/stack.h
