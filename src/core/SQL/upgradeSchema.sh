#!/bin/bash

TARGET_DIR=$1
if [ ! $TARGET_DIR ]; then
    TARGET_DIR=`pwd`
fi

if [[ ! -d $TARGET_DIR ]]; then
    echo "No such directory $TARGET_DIR"
    exit 1
fi

cd $TARGET_DIR

CURRENT_VERSION=$(ls | sort -n | tail -n1)

if [ ! $CURRENT_VERSION ]; then
    echo "no previous schema found to upgrade from"
    exit 2
fi

((NEW_VERSION=$CURRENT_VERSION + 1))

mkdir $NEW_VERSION
git add $NEW_VERSION
find $CURRENT_VERSION -depth 1 \! -name "upgrade_*" \! -name ".*" -exec git mv {} $NEW_VERSION \;
cp ${CURRENT_VERSION}/upgrade_999_version.sql $NEW_VERSION
git add ${NEW_VERSION}/upgrade_999_version.sql
