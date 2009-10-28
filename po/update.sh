#!/usr/bin/env sh
if [ ! $# -eq 1 ]; then
  exec >&2
  echo "Usage: $0 <language>"
  echo "  language: two-letter language code + country code if applicable (de, en_GB)"
  exit 1
fi

CONV=lconvert
BASE=quassel_$1
PO=$BASE.po
TS=$BASE.ts

$CONV -i $PO -o $TS   &&
  lupdate ../src -ts $TS &&
  $CONV -i $TS -o $PO

# remove cruft
rm ${TS}
