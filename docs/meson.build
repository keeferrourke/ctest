doxyfile = configure_file(input: 'Doxyfile.in', output: 'Doxyfile',
    configuration: cdata, install: false)

docs_out = cdata.get('DOCS_OUTPUT_DIR')
latex_dir = join_paths(docs_out, 'latex')
doc_name = 'refman.pdf'
make_docs = [
    'make_docs.sh',
    doxyfile,
    docs_out,
    join_paths(latex_dir, doc_name)
]

run_target('docs', command: make_docs)
