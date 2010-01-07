#!/usr/bin/env sh
if [ ! $# -eq 1 ]; then
  exec >&2
  echo "Usage: $0 <language>"
  echo "  language: two-letter language code + country code if applicable (de, en_GB)"
  exit 1
fi

CONV=lconvert
POT=quassel.pot
BASE=quassel_$1
PO=$BASE.po
TS=$BASE.ts

( [ -f $PO ] || ( [ -f $POT ] && cp $POT $PO ) ) &&
  $CONV -i $PO -o $TS &&
  lupdate -no-obsolete ../src -ts $TS &&
  $CONV -i $TS -o $PO &&
  rm $TS

[ $? -ne 0 ] && echo "Something went wrong"
