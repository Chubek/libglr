project(
  'glrpp',
  'cpp', 'c',
  version: '0.1.0',
  default_options: ['cpp_std=c++20', 'c_std=c11']
)

build_dep = get_option('BUILD_DEP')
build_rpm = get_option('BUILD_RPM')
build_tarball = get_option('BUILD_TARBALL')
build_zlib = get_option('BUILD_ZLIB')
build_docs = get_option('BUILD_DOCS')
install_glrpp = get_option('GLRPP_INSTALL')
install_bundled_libltdl = get_option('GLRPP_INSTALL_BUNDLED_LIBLTDL')

cc = meson.get_compiler('c')
python = import('python').find_installation('python3', required: false)

scripts_dir = meson.current_source_dir() / 'scripts'
archive_script = scripts_dir / 'archive-services.py'
docstring_script = scripts_dir / 'docstring-services.py'

glrpp_inc = include_directories(
  'include',
  'third_party/brigand/include',
  'third_party/compile-time-regular-expressions/include',
  'third_party/frozen/include',
  'third_party/libltdl',
  'third_party/libltdl/libltdl',
  'third_party/magic_enum/include',
  'third_party/nameof/include',
  'third_party/pfr/include'
)

libltdl_inc = include_directories('third_party/libltdl', 'third_party/libltdl/libltdl')
libltdl_sources = files(
  'third_party/libltdl/ltdl.c',
  'third_party/libltdl/lt_dlloader.c',
  'third_party/libltdl/lt_error.c',
  'third_party/libltdl/lt__alloc.c',
  'third_party/libltdl/lt__argz.c',
  'third_party/libltdl/lt__dirent.c',
  'third_party/libltdl/lt__strl.c',
  'third_party/libltdl/slist.c',
  'third_party/libltdl/loaders/preopen.c',
)

if host_machine.system() == 'darwin'
  libltdl_sources += files('third_party/libltdl/loaders/dyld.c')
elif host_machine.system() == 'windows'
  libltdl_sources += files('third_party/libltdl/loaders/loadlibrary.c')
else
  libltdl_sources += files('third_party/libltdl/loaders/dlopen.c')
endif

libdl_dep = cc.find_library('dl', required: false)
libltdl = library(
  'ltdl',
  libltdl_sources,
  include_directories: libltdl_inc,
  dependencies: libdl_dep.found() ? [libdl_dep] : [],
  install: install_bundled_libltdl,
)

glrpp_dep = declare_dependency(
  include_directories: glrpp_inc,
  link_with: libltdl,
  dependencies: libdl_dep.found() ? [libdl_dep] : [],
)

if install_glrpp
  install_subdir('include/glrpp', install_dir: get_option('includedir'))
  install_subdir('third_party/brigand/include/brigand', install_dir: get_option('includedir'))
  install_subdir('third_party/compile-time-regular-expressions/include/ctre', install_dir: get_option('includedir'))
  install_data(
    'third_party/compile-time-regular-expressions/include/ctre.hpp',
    'third_party/compile-time-regular-expressions/include/ctre-unicode.hpp',
    'third_party/compile-time-regular-expressions/include/unicode-db.hpp',
    install_dir: get_option('includedir')
  )
  install_subdir('third_party/frozen/include/frozen', install_dir: get_option('includedir'))
  install_subdir('third_party/libltdl/libltdl', install_dir: get_option('includedir'))
  install_data('third_party/libltdl/ltdl.h', install_dir: get_option('includedir'))
  install_subdir('third_party/magic_enum/include/magic_enum', install_dir: get_option('includedir'))
  install_subdir('third_party/nameof/include/nameof', install_dir: get_option('includedir'))
  install_subdir('third_party/pfr/include/boost', install_dir: get_option('includedir'))

  pkg = import('pkgconfig')
  pkg.generate(
    name: 'glrpp',
    description: 'Header-only C++20 DSL wrapper for libglr with libltdl-based runtime loading',
    version: meson.project_version(),
    subdirs: '.',
    libraries_private: [libltdl],
    extra_cflags: ['-I${includedir}'],
  )
endif

if build_dep and python.found()
  run_target('deb', command: [python, archive_script, 'deb'])
endif
if build_rpm and python.found()
  run_target('rpm', command: [python, archive_script, 'rpm'])
endif
if build_tarball and python.found()
  run_target('tarball', command: [python, archive_script, 'tarball'])
endif
if build_zlib and python.found()
  run_target('zlib', command: [python, archive_script, 'zlib'])
endif

if build_docs and python.found()
  run_target('html', command: [python, docstring_script, 'html'])
  run_target('singlehtml', command: [python, docstring_script, 'singlehtml'])
  run_target('pdf', command: [python, docstring_script, 'pdf'])
  run_target('latex', command: [python, docstring_script, 'latex'])
endif
