#!/bin/bash
# Copyright (C) 2005-2022 by the Quassel Project - devel@quassel-irc.org
# Licensed under GNU General Public License version 2, or (at your option)
# version 3.
#
# Call this script whenever you need to add a new database schema version.
# You can also simply create a new folder in '[SQLite/PostgreSQL]/version/##',
# where '##' represents your new version number, incremented by one from
# whatever the current latest version is (e.g. 21 -> 22).
#
# NOTE: If you upgrade the database schema version, you must add upgrade
# scripts and modify the existing setup queries.  See 'README.md' for details.

TARGET_DIR="$1"
# If not specified, assume current directory
if [ ! "$TARGET_DIR" ]; then
    TARGET_DIR="$(pwd)"
fi

if [[ ! -d "$TARGET_DIR" ]]; then
    echo "No such directory '$TARGET_DIR'" >&2
    exit 1
fi

# Find out the name of the target directory to offer some guidance later.
TARGET_DB_NAME=$(basename "$TARGET_DIR")

# Upgrade scripts are stored in the 'version' subdirectory, e.g.
# 'SQL/[database]/version/##'.
cd "$TARGET_DIR/version"

# Grab the current schema version
CURRENT_VERSION=$(ls | sort -n | tail -n1)

if [ ! $CURRENT_VERSION ]; then
    echo "No previous schema found to upgrade from" >&2
    exit 2
fi

# Increment by one
((NEW_VERSION=$CURRENT_VERSION + 1))

# Create the new schema directory.
mkdir "$NEW_VERSION"
# Git doesn't track empty folders, no need for 'git add "$NEW_VERSION"'.

echo "New schema version '$TARGET_DB_NAME/version/$NEW_VERSION' created." >&2
echo "Create any needed 'upgrade_[...].sql' scripts in this folder." >&2

# Don't move any files over.  Schema version upgrade scripts are now stored
# independently of the main SQL files in order to make the repository history
# more useful and easier to work with.

# Granted, this script doesn't do anything one couldn't easily manually do.
# I'd argue that's a good thing.  Though as this script offers documentation
# and guidance for contributors new to the database schema system as well as
# helping migrate those used to the older method, it seems worthwhile keeping.
