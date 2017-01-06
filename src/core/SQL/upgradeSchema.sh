#!/bin/bash
# Copyright (C) 2005-2016 by the Quassel Project - devel@quassel-irc.org
# Licensed under GNU General Public License version 2, or (at your option)
# version 3.
#
# "One does not simply 'upgrade the schema'..."
#
# When changing Quassel's database schema, you need to follow several steps to
# handle all cases (upgrade, Postgres migration, etc).
#
# 1.  Run this script on -both- the PostgreSQL and SQLite directory
#
# Make sure you're on the Git branch you want to modify
# > ./upgradeSchema.sh "PostgreSQL"
# > ./upgradeSchema.sh "SQLite"
#
# 2.  Modify queries and setup scripts to handle your change
#
# [Example] Modifying the 'ircserver' table to add column 'test'
# Modify all query/setup .sql files that touch the 'ircserver' table for
# -both- PostgreSQL and SQLite.
#
# 3.  Create an upgrade script for -both- PostgreSQL and SQLite
#
# [Example] Modifying the 'ircserver' table to add column 'test'
# Add the file 'upgrade_000_alter_ircserver_add_test.sql' with contents:
# > ALTER TABLE ircserver
# > ADD COLUMN test [additional column-specific details]
#
# 4.  Create a pair of migration scripts for moving from SQLite to PostgreSQL
#
# [Example] Modifying the 'ircserver' table to add column 'test'
# > Modify 'SQLite/##/migrate_read_ircserver.sql' to select from new column
# > Modify 'PostgreSQL/##/migrate_write_ircserver.sql' to insert to new column
#
# 5.  Update the SQL resource file; re-run CMake if needed
#
# The easy way: run "updateSQLResource.sh" in this directory.
#
# The manual way:
# Add the new SQL queries to 'src/core/sql.qrc', update all existing files.
#
# [Example] Modifying the 'ircserver' table to add column 'test'
# > Add the new upgrade script...
#   <file>./SQL/SQLite/19/upgrade_000_alter_ircserver_add_test.sql</file>
#   <file>./SQL/PostgreSQL/18/upgrade_000_alter_ircserver_add_test.sql</file>
# > Find/replace all non-upgrade scripts from the old schema number to new one
#   <file>./SQL/SQLite/[18->19]/update_buffer_persistent_channel.sql</file>
#   <file>./SQL/PostgreSQL/[17->18]/update_buffer_persistent_channel.sql</file>
#   (etc)
#
# 6.  Update the migration logic in 'src/core/abstractsqlstorage.h', and the
# storage backends 'postgresqlstorage.cpp' and 'sqlitestorage.cpp'
#
# [Example] Modifying the 'ircserver' table to add column 'test'
# > Modify struct 'IrcServerMO' in 'abstractsqlstorage.h', adding an entry for
#   'test' of the appropriate data-type.
# > Modify 'SqliteMigrationReader::readMo(IrcServerMO &ircserver)' in
#   'sqlitestorage.cpp' to read from the new column and store it in the
#   migration object.  You may need to convert from SQLite's looser types.
# > Modify 'PostgreSqlMigrationWriter::writeMo(const IrcServerMO &ircserver)'
#   in 'postgresqlstorage.cpp' to write to the new column from the data in the
#   migration object.
#
# 7.  Update any affected queries in storage backends 'postgresqlstorage.cpp'
# and 'sqlitestorage.cpp', and any related synchronized 'src/common' classes.
#
# [Example] Modifying the 'ircserver' table to add column 'test'
# > Update 'network.h' to add new column to Server structure
#   QString proxyPass;                                     // Existing code
#   Typename test;                                         // New column 'test'
#   [...]
#   Server() : port(6667), ..., proxyPort(8080), test("defaultValue") {}
# > Modify reading data in ____Storage::networks(...)
#   server.proxyPass = serversQuery.value(10).toString();  // Existing code
#   server.test = serversQuery.value(11).toType();         // New column 'test'
#   servers << server;                                     // Existing code
# > Modify writing data in ____Storage::bindServerInfo(...)
#   query.bindValue(":proxypass", server.proxyPass);       // Existing code
#   query.bindValue(":test", server.test);                 // New column 'test'
#
# 8.  If protocol changed (add a setting, etc), add a new "Feature" flag
#
# Newer clients need to detect when they're on an older core to disable the
# feature.  Use 'enum Feature' in 'quassel.h'.  In client-side code, test with
# 'if (Client::coreFeatures() & Quassel::FeatureName) { ... }'
#
# 9.  Test everything!  Upgrade, migrate, new setups, new client/old core,
# old client/new core, etc.

TARGET_DIR="$1"
# If not specified, assume current directory
if [ ! "$TARGET_DIR" ]; then
    TARGET_DIR="$(pwd)"
fi

if [[ ! -d "$TARGET_DIR" ]]; then
    echo "No such directory '$TARGET_DIR'"
    exit 1
fi

cd "$TARGET_DIR"

# Grab the current schema version
CURRENT_VERSION=$(ls | sort -n | tail -n1)

if [ ! $CURRENT_VERSION ]; then
    echo "No previous schema found to upgrade from"
    exit 2
fi

# Increment by one
((NEW_VERSION=$CURRENT_VERSION + 1))

# Create the new schema directory, add the directory, and move all files over...
mkdir "$NEW_VERSION"
git add "$NEW_VERSION"
# ...except for 'upgrade_' scripts.
find "$CURRENT_VERSION" -maxdepth 1 -type f \! -name "upgrade_*" \! -name ".*" -exec git mv {} "$NEW_VERSION" \;
