#!/bin/bash

tx pull $* &&
git add po/*.po && (
  translators=$(while read mode pofile; do
    translator=$(git diff --cached -- ${pofile} | perl -le 'while (<>) { if (/^\+(?:#|.*?:) *(.*?)(<[^@>]+@[^>]+>)/p) { $xltrs{$2} = $1 unless $xltrs{$2}; } last if /^\+"Last-Translator: /; }; push(@out, $n.$e) while (($e, $n) = each %xltrs); print(join(", ", @out));')
    lang=${pofile%.po}
    lang=${lang#po/}
    echo " - ${lang}: ${translator}"
  done < <(git status --porcelain po/*.po | egrep '^[AM]  '))

  git commit -em "Update translations from Transifex

Many thanks to:
${translators}" po/*.po
)
