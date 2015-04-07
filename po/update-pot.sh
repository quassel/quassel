#!/bin/bash

po_dir=$(cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd)
src_dir="${po_dir}/../src"
quassel_ts="${po_dir}/quassel.ts"
quassel_po="${po_dir}/quassel.po"
quassel_pot="${quassel_po}t"
quassel_pot_patch="${quassel_pot}.patch"

lupdate ${src_dir} -ts ${quassel_ts} && lconvert -i ${quassel_ts} -o ${quassel_po}    \
  && msguniq -o ${quassel_pot} ${quassel_po} && rm ${quassel_ts} ${quassel_po}        \
  && patch -d ${po_dir} -Np2 < ${quassel_pot_patch}                                   \
  && sed -i -re 's/^msgstr\[0\] ""/msgstr[0] ""\nmsgstr[1] ""/;' ${quassel_pot_patch}
