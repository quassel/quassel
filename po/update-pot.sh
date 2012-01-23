#!/bin/sh

lupdate ../src -ts quassel.ts && lconvert -i quassel.ts -o quassel.po        \
  && msguniq -o quassel.pot quassel.po && rm quassel.ts quassel.po           \
  && patch -Np2 < quassel.pot.patch                                          \
  && sed -i -re 's/^msgstr\[0\] ""/msgstr[0] ""\nmsgstr[1] ""/;' quassel.pot
