project('GLRpp', 'cpp',
  version: '1.0.0',
  license: 'MIT',
  default_options: ['cpp_std=c++20', 'warning_level=3']
)

# Header-only library
glrpp_inc = include_directories('.')

glrpp_dep = declare_dependency(
  include_directories: glrpp_inc
)

# Install headers
install_headers(
  'GLRpp.hpp',
  'Polyfills.hpp',
  'dslutils.hpp',
  subdir: 'GLRpp'
)

# pkg-config file
pkg = import('pkgconfig')
pkg.generate(
  name: 'GLRpp',
  description: 'Modern C++ wrapper for libglr',
  version: meson.project_version(),
  subdirs: 'GLRpp'
)

# Optional: Find libglr for validation
libglr_dep = dependency('libglr', required: false)

# Doxygen documentation
doxygen = find_program('doxygen', required: false)
if doxygen.found()
  doxyfile_conf = configuration_data()
  doxyfile_conf.set('PROJECT_NAME', meson.project_name())
  doxyfile_conf.set('PROJECT_VERSION', meson.project_version())
  doxyfile_conf.set('PROJECT_SOURCE_DIR', meson.current_source_dir())
  doxyfile_conf.set('PROJECT_BINARY_DIR', meson.current_build_dir())
  
  doxyfile = configure_file(
    input: 'Doxyfile.in',
    output: 'Doxyfile',
    configuration: doxyfile_conf
  )
  
  custom_target('docs',
    input: doxyfile,
    output: 'html',
    command: [doxygen, '@INPUT@'],
    build_by_default: false
  )
endif()
