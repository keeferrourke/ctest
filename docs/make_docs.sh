#!/bin/sh

cd "${MESON_SOURCE_ROOT}"

doxyfile=$1
output_dir=$2
pdf_doc=$3

doxygen $doxyfile
make -C $(dirname $pdf_doc)
mv $pdf_doc $output_dir
rm -r $(dirname $pdf_doc)
