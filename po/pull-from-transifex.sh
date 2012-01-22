#!/bin/bash

tx pull &&
git add po/*.po && (
  translators=$(while read mode pofile; do
    translator=$(perl -ne 's/^"Last-Translator: (.*?)(?:\\n)?"$/\1/ && print $1;' ${pofile})
    lang=${pofile%.po}
    lang=${lang#po/}
    echo " - ${lang}: ${translator}"
  done < <(git status --porcelain po/*.po | egrep '^[AM]  '))

  git commit -m "Update translations from Transifex

Many thanks to:
${translators}" po/*.po
)
