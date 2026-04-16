project(
  'glrpp',
  'cpp', 'c',
  version: '0.1.0',
  default_options: ['cpp_std=c++20']
)

# ============================================================
# Meson build options
# ============================================================

build_dep      = get_option('BUILD_DEP')
build_rpm      = get_option('BUILD_RPM')
build_tarball  = get_option('BUILD_TARBALL')
build_zlib     = get_option('BUILD_ZLIB')
build_docs     = get_option('BUILD_DOCS')
build_man      = get_option('TARGET_MANPAGES')
build_bindings = get_option('GLRPP_BUILD_BINDINGS')
install_libs   = get_option('GLRPP_INSTALL')

python = import('python').find_installation('python3')

scripts_dir = meson.current_source_dir() / 'glrpp' / 'scripts'
archive_script    = scripts_dir / 'archive-services.py'
docstring_script  = scripts_dir / 'docstring-services.py'

# ============================================================
# Header-only glrpp library
# ============================================================

glrpp_inc = include_directories(
  'include',
  'third_party/brigand/include',
  'third_party/ctre/include',
  'third_party/frozen/include',
  'third_party/magic_enum/include',
  'third_party/nameof/include',
  'third_party/pfr/include'
)

glrpp_lib = library(
  'glrpp',
  [],                       # header-only
  include_directories: glrpp_inc,
  install: install_libs,
  install_dir: get_option('includedir'),
  link_whole: true,
  cpp_args: ['-DGLRPP_HEADER_ONLY']
)

# ============================================================
# SWIG Bindings (optional)
# ============================================================

if build_bindings
  swig = find_program('swig', required: true)
  libglr = cc.find_library('glr', required: true)
  glr_header = cc.find_header('glr.h', required: true)

  swig_out = custom_target(
    'glrpp_glr_bindings_sources',
    input: 'bindings/glr.i',
    output: ['glrpp_glr_bindings.cpp', 'glrpp_glr_bindings.h'],
    command: [swig, '-c++', '-python', '-o', '@OUTPUT0@', '@INPUT@'],
    depend_files: ['bindings/glr.i']
  )

  bindings_lib = shared_library(
    'glrpp_glr_bindings',
    swig_out,
    include_directories: [glrpp_inc],
    dependencies: [libglr],
    install: install_libs
  )
endif

# ============================================================
# Installation of headers + CMake package metadata
# ============================================================

if install_libs

  # headers
  install_subdir(
    'include/glrpp',
    install_dir: 'include'
  )

  # third-party bundle (header-only)
  install_subdir(
    'third_party',
    install_dir: 'include/glrpp'
  )

  # CMake package files
  install_data(
    'cmake/glrppConfig.cmake',
    'cmake/glrppConfigVersion.cmake',
    'cmake/glrppTargets.cmake',
    install_dir: join_paths(get_option('libdir'), 'cmake/glrpp')
  )
endif

# ============================================================
# Packaging targets (deb / rpm / tarball / zlib)
# ============================================================

if build_dep
  custom_target(
    'deb',
    command: [python, archive_script, 'deb'],
    build_by_default: false
  )
endif

if build_rpm
  custom_target(
    'rpm',
    command: [python, archive_script, 'rpm'],
    build_by_default: false
  )
endif

if build_tarball
  custom_target(
    'tarball',
    command: [python, archive_script, 'tarball'],
    build_by_default: false
  )
endif

if build_zlib
  custom_target(
    'zlib',
    command: [python, archive_script, 'zlib'],
    build_by_default: false
  )
endif

# ============================================================
# Documentation targets (html, pdf, latex, singlehtml)
# ============================================================

if build_docs

  custom_target('html',
    command: [python, docstring_script, 'html'],
    build_by_default: false
  )

  custom_target('singlehtml',
    command: [python, docstring_script, 'singlehtml'],
    build_by_default: false
  )

  custom_target('pdf',
    command: [python, docstring_script, 'pdf'],
    build_by_default: false
  )

  custom_target('latex',
    command: [python, docstring_script, 'latex'],
    build_by_default: false
  )

  custom_target('install-docs',
    command: [python, docstring_script, 'install'],
    build_by_default: false
  )

  # ----------------------------------------------------------
  # Bundle documentation via GLRPP_BUNDLE_DOCS env var
  # ----------------------------------------------------------

  docs_env = meson.get_env('GLRPP_BUNDLE_DOCS')
  if docs_env != ''
    custom_target(
      'bundle-docs',
      command: [python, archive_script, 'bundle', docs_env],
      build_by_default: false
    )
  endif
endif

# ============================================================
# Manpage targets
# ============================================================

if build_man

  nuroff = find_program('nuroff', required: false)

  custom_target(
    'build-man',
    command: [nuroff, meson.current_source_dir() / 'glrpp/man'],
    build_by_default: false
  )

  custom_target(
    'install-man',
    command: [
      'cp', '-r',
      meson.current_source_dir() / 'glrpp/man',
      join_paths(get_option('prefix'), 'share/man')
    ],
    build_by_default: false
  )
endif
