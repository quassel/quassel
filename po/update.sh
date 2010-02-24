#!/usr/bin/env sh
if [ ! $# -eq 1 ]; then
  exec >&2
  echo "Usage: $0 <language>"
  echo "  language: two-letter language code + country code if applicable (de, en_GB)"
  exit 1
fi

msgmerge --no-wrap --width 1 -U $1.po quassel.pot
[ $? -ne 0 ] && echo "Something went wrong"
