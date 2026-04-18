fs = import('fs')

doxygen = find_program('doxygen', required: true)
make_program = find_program('gmake', 'make', required: true)
awk = find_program('awk', required: true)
sh = find_program('sh', required: true)

manifest_path = meson.current_source_dir() / 'booklet' / 'Manifest.yaml'
manifest_result = run_command(
  awk,
  '/^[[:space:]]*- file:[[:space:]]*/ { print $3 }',
  manifest_path,
  check: true
)

booklet_input = ''
seen_chapters = []
foreach chapter : manifest_result.stdout().strip().split('\n')
  if chapter == ''
    continue
  endif
  chapter_path = meson.current_source_dir() / 'booklet' / chapter
  if fs.exists(chapter_path)
    booklet_input += 'INPUT += "@0@"\n'.format(chapter_path)
    seen_chapters += [chapter]
  endif
endforeach

extra_result = run_command(
  sh,
  '-c',
  'find "$1" -maxdepth 1 -name "*.md" -printf "%f\n" | sort',
  '_',
  meson.current_source_dir() / 'booklet',
  check: true
)

foreach chapter : extra_result.stdout().strip().split('\n')
  if chapter == '' or seen_chapters.contains(chapter)
    continue
  endif
  chapter_path = meson.current_source_dir() / 'booklet' / chapter
  booklet_input += 'INPUT += "@0@"\n'.format(chapter_path)
endforeach

doxyfile_conf = configuration_data()
doxyfile_conf.set('GLRPP_PROJECT_NAME', meson.project_name())
doxyfile_conf.set('GLRPP_PROJECT_VERSION', meson.project_version())
doxyfile_conf.set('GLRPP_PROJECT_SOURCE_DIR', meson.project_source_root())
doxyfile_conf.set('GLRPP_DOCS_SOURCE_DIR', meson.current_source_dir())
doxyfile_conf.set('GLRPP_DOCS_OUTPUT_DIR', meson.current_build_dir())
doxyfile_conf.set('GLRPP_BOOKLET_INPUT', booklet_input.strip())

configure_file(
  input: 'Doxyfile.in',
  output: 'Doxyfile',
  configuration: doxyfile_conf,
  format: 'cmake'
)

doxyfile_path = meson.current_build_dir() / 'Doxyfile'
html_command = '@0@ "@1@"'.format(doxygen.full_path(), doxyfile_path)
pdf_command = '@0@ "@1@" && cd "@2@/latex" && "@3@" && cp "refman.pdf" "../glrpp-reference.pdf"'.format(
  doxygen.full_path(),
  doxyfile_path,
  meson.current_build_dir(),
  make_program.full_path()
)

run_target('html', command: [sh, '-c', html_command])
run_target('latex', command: [sh, '-c', html_command])
run_target('docs', command: [sh, '-c', html_command])
run_target('pdf', command: [sh, '-c', pdf_command])
