#!/usr/bin/env bash
quasselsrc=/usr/src/quassel
localpobranch=i18n-tx-sync

pushd "$quasselsrc" && ( 
  currb=$(git name-rev --name-only HEAD)
  git checkout -q $localpobranch && (
    EDITOR=/bin/true VISUAL=/usr/bin/editor "$quasselsrc"/po/pull-from-transifex.sh -f &&
      git push -q
  ); git checkout -q "$currb"
); popd
