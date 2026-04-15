AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/third_party/sfsexp/src -I$(top_srcdir)/tests

check_PROGRAMS = \
  test_ambiguity \
  test_disambiguate \
  test_forest \
  test_fork \
  test_grammar \
  test_parser \
  test_reader \
  test_reduction \
  test_rewrite \
  test_stack

TESTS = $(check_PROGRAMS)

test_ambiguity_SOURCES = test_ambiguity.c
test_ambiguity_LDADD = $(top_builddir)/libglr.la

test_disambiguate_SOURCES = test_disambiguate.c
test_disambiguate_LDADD = $(top_builddir)/libglr.la $(top_builddir)/disambstd/libglrdisambstd.a

test_forest_SOURCES = test_forest.c
test_forest_LDADD = $(top_builddir)/libglr.la

test_fork_SOURCES = test_fork.c
test_fork_LDADD = $(top_builddir)/libglr.la

test_grammar_SOURCES = test_grammar.c
test_grammar_LDADD = $(top_builddir)/libglr.la

test_parser_SOURCES = test_parser.c
test_parser_LDADD = $(top_builddir)/libglr.la

test_reader_SOURCES = test_reader.c
test_reader_LDADD = $(top_builddir)/libglr.la

test_reduction_SOURCES = test_reduction.c
test_reduction_LDADD = $(top_builddir)/libglr.la

test_rewrite_SOURCES = test_rewrite.c
test_rewrite_LDADD = $(top_builddir)/libglr.la

test_stack_SOURCES = test_stack.c
test_stack_LDADD = $(top_builddir)/libglr.la
