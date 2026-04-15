project('libglr', 'c', version: '1.0.0', default_options: ['c_std=c11', 'warning_level=2'])

build_tests = get_option('build_tests')
build_examples = get_option('build_examples')
build_disambstd = get_option('build_disambstd')
build_rewritelib = get_option('build_rewritelib')

inc = include_directories('include', 'third_party/sfsexp/src')

sfsexp = static_library(
  'sfsexp',
  files(
    'third_party/sfsexp/src/cstring.c',
    'third_party/sfsexp/src/event_temp.c',
    'third_party/sfsexp/src/faststack.c',
    'third_party/sfsexp/src/io.c',
    'third_party/sfsexp/src/parser.c',
    'third_party/sfsexp/src/sexp.c',
    'third_party/sfsexp/src/sexp_memory.c',
    'third_party/sfsexp/src/sexp_ops.c',
    'third_party/sfsexp/src/sexp_vis.c',
  ),
  include_directories: inc,
  install: true,
)

libglr = static_library(
  'glr',
  files(
    'src/glr/disambiguate.c',
    'src/glr/forest.c',
    'src/glr/fork.c',
    'src/glr/grammar.c',
    'src/glr/graph.c',
    'src/glr/parser.c',
    'src/glr/reduction.c',
    'src/glr/rewrite.c',
    'src/glr/stack.c',
  ),
  include_directories: inc,
  link_with: sfsexp,
  install: true,
)

install_headers(
  'include/glr/disambiguate.h',
  'include/glr/forest.h',
  'include/glr/fork.h',
  'include/glr/glr.h',
  'include/glr/grammar.h',
  'include/glr/graph.h',
  'include/glr/parser.h',
  'include/glr/reduction.h',
  'include/glr/rewrite.h',
  'include/glr/stack.h',
  subdir: 'glr',
)

if build_tests
  test('test_rewrite', executable('test_rewrite', 'tests/test_rewrite.c',
    include_directories: inc,
    link_with: libglr))
endif

if build_examples
  executable('ambiguous', 'examples/ambiguous.c', include_directories: inc, link_with: libglr)
  executable('calc', 'examples/calc.c', include_directories: inc, link_with: libglr)
endif

if build_disambstd
  static_library('glrdisambstd',
    files(
      'disambstd/associativity.c',
      'disambstd/dynamic_programming.c',
      'disambstd/precedence.c',
      'disambstd/predicate.c',
      'disambstd/probability.c',
      'disambstd/semantic.c',
    ),
    include_directories: inc,
    link_with: libglr,
    install: true,
  )
endif

if build_rewritelib
  install_data(
    'rewritelib/eiminate-ambguity.grl',
    'rewritelib/eliminate-epsilon-production.grl',
    'rewritelib/eliminate-useless-symbols.grl',
    'rewritelib/left-factor.grl',
    'rewritelib/make-lr-compat.grl',
    'rewritelib/remove-left-recursion.grl',
    'rewritelib/remove-unit-productions.grl',
    install_dir: join_paths(get_option('datadir'), 'libglr', 'rewritelib'),
  )
endif
