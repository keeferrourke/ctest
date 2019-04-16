project('libtdd', 'c', version: '1.0.0', license: 'ISC',
    default_options: [
        'c_std=c99',
])
posix_c_source = get_option('posix_c_source')
if posix_c_source != ''
    add_global_arguments('-D_POSIX_C_SOURCE='+posix_c_source,
                         language: 'c')
endif

if get_option('use_colour_output')
    add_project_arguments('-DUSE_COLOUR', language: 'c')
endif


cdata = configuration_data()
cdata.set('PROJECT_NAME', meson.project_name())
cdata.set('VERSION', meson.project_version())
cdata.set('TOP_SRC_DIR', meson.source_root())
cdata.set('INCLUDE_DIR', join_paths(meson.source_root(), 'include'))
cdata.set('DOCS_OUTPUT_DIR', join_paths(meson.build_root(), 'docs'))
cdata.set('README_PATH', join_paths(meson.source_root(), 'REAMDE.md'))

threads = dependency('threads')

project_sources = []
project_api_headers = []
project_headers = []
project_includes = []
project_objects = []
example_sources = []

subdir('src')       # populate project_sources
subdir('include')   # populate project_includes


# if doxygen found, generate documentation
doxygen = find_program('doxygen', required: false)
if doxygen.found()
  cdata.set('HAS_DOT', find_program('dot', required: false).found() ? 'YES' : 'NO')
  cdata.set('HTML_PAGES', get_option('generate_html_docs') ? 'YES' : 'NO')
  cdata.set('GEN_LATEX', get_option('generate_pdf_docs') ? 'YES' : 'NO')
  if get_option('generate_man_pages')
      cdata.set('MAN_PAGES', 'YES')
      man_dir = join_paths(cdata.get('DOCS_OUTPUT_DIR'), 'man', 'man3')
      install_subdir(man_dir, install_dir: 'man')
  else
      cdata.set('MAN_PAGES', 'NO')
  endif
  subdir('docs') 
endif

project_headers += project_api_headers

install_headers(project_api_headers)
lib = library('tdd', install: true, sources: project_sources,
    include_directories: project_includes, dependencies: threads)

run_target('format', command: [
    'clang-format',
    '-i',
    '-style=file',
    project_sources,
    project_headers,
    example_sources
])

subdir('examples')
executable('example', example_sources, link_with: lib,
    include_directories: project_includes)
