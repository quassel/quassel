#!/bin/bash
# Copyright (C) 2005-2016 by the Quassel Project - devel@quassel-irc.org
# Licensed under GNU General Public License version 2, or (at your option)
# version 3.
#
# Call this script whenever you move or rename the SQL files inside this
# folder.  It'll regenerate 'sql.qrc' for you.  If it fails, you can manually
# edit the file, too; just follow the pattern already in there.
#
# NOTE: In most cases you must upgrade the database schema version to make a
# change, which includes adding upgrade scripts and modifying existing setup
# queries.  See 'upgradeSchema.sh' for details.

set -u # Bash - fail on undefined variable

# Path to the directory containing this script, resolving any symlinks, etc
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Name of the current directory
SQL_DIR_NAME="${SCRIPT_DIR##*/}"
# See https://stackoverflow.com/questions/1371261/get-current-directory-name-without-full-path-in-bash-script/1371283#1371283

# SQL resource file (one directory up from this script)
SQL_RES_FILE="$SCRIPT_DIR/../sql.qrc"

# Add the Qt resource header, overwriting the existing file
cat > "$SQL_RES_FILE" <<EOF
<!DOCTYPE RCC><RCC version="1.0">
<qresource>
EOF

# In a subshell, change to the SQL directory so find output remains consistent,
# then...
# Find all files with a .sql ending...
# | ...sort the results by version
# | ...modify the beginning to change '.[...]' into '    <file>./SQL[...]' (add the directory path back)
#   ...and modify the end to add '[...]</file>'
# | ...append to the resource file.
(cd "$SCRIPT_DIR" ; find . -name "*.sql" | sort -t/ -k2,2 -k3n | sed -e "s/^./    <file>.\/$SQL_DIR_NAME/g" -e "s/$/<\/file>/g" >> "$SQL_RES_FILE" )
# Newer versions of sort support a "--version-sort" flag to naturally sort.
# Unfortunately, some supported platforms don't yet have that version.
# For now, "-t/ -k2,2 -k3n" does -almost- the same thing (there's a difference
# when sorting periods versus underscores).

# Add the Qt resource footer
cat >> "$SQL_RES_FILE" <<EOF
</qresource>
</RCC>
EOF
