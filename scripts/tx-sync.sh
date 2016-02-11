#!/usr/bin/env bash
set -u
quasselsrc=/usr/src/quassel
localpobranch=i18n-tx-sync
remote=origin
branch=master

pushd "$quasselsrc" && ( 
  currb=$(git name-rev --name-only HEAD)
  git checkout -q $localpobranch && (
    git pull -q --no-edit $remote $branch &&
    pushd po/ &&
    ./update-pot.sh &&
    git commit -qm 'Update quassel.pot' ${quasselsrc}/po/quassel.pot
    popd &&
    "$quasselsrc"/po/pull-from-transifex.sh -f &&
    git push -q
  ); git checkout -q "$currb"
); popd
