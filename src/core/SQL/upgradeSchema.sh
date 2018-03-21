#!/bin/bash
# Copyright (C) 2005-2016 by the Quassel Project - devel@quassel-irc.org
# Licensed under GNU General Public License version 2, or (at your option)
# version 3.
#
# "One does not simply 'upgrade the schema'..."
#
# When changing Quassel's database schema, you need to follow several steps to
# handle all cases (upgrade, Postgres migration, etc).  Some of these steps may
# not apply to all schema upgrades.
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
# 3.  Create upgrade scripts for -both- PostgreSQL and SQLite
#
# They should go in the newest "[...]/version/##" folders that were created by
# running this script.  Don't modify the files in lower-numbered version
# folders.
#
# [Example] Modifying the 'ircserver' table to add column 'test'
# Add the file 'upgrade_000_alter_ircserver_add_test.sql' with contents:
# > ALTER TABLE ircserver
# > ADD COLUMN test [additional column-specific details]
#
# 4.  Update the pair of migration scripts for moving from SQLite to PostgreSQL
#
# [Example] Modifying the 'ircserver' table to add column 'test'
# > Modify 'SQLite/migrate_read_ircserver.sql' to select from new column
# > Modify 'PostgreSQL/migrate_write_ircserver.sql' to insert to new column
#
# 5.  Update the SQL resource file; re-run CMake if needed
#
# The easy way: run "updateSQLResource.sh" in this directory.
#
# The manual way:
# Add the new SQL queries to 'src/core/sql.qrc', update all changed existing
# files.
#
# [Example] Modifying the 'ircserver' table to add column 'test'
# > Add the new upgrade scripts...
#   <file>./SQL/SQLite/version/19/upgrade_000_alter_ircserver_add_test.sql</file>
#   <file>./SQL/PostgreSQL/version/18/upgrade_000_alter_ircserver_add_test.sql</file>
# > Add/update non-upgrade scripts, if any...
#   <file>./SQL/SQLite/update_buffer_persistent_channel.sql</file>
#   <file>./SQL/PostgreSQL/update_buffer_persistent_channel.sql</file>
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
# 'if (Client::isCoreFeatureEnabled(Quassel::Feature::FeatureName)) { ... }'
#
# 9.  Test everything!  Upgrade, migrate, new setups, new client/old core,
# old client/new core, etc.
#
# More specifically, you likely should try the following combinations,
# especially if you change the protocol.  Check if any data or settings get
# lost or corrupted, or if anything unusual shows up in the log.
#
# [Mac/Linux]
# Fresh configuration (reset the database and settings)
# > SQLite
#   > New core, new client
#   > New core, old client
#   > Old core, new client
#   > New monolithic (combined core/client build)
# > Postgres
#   > New core, new client
#   > New core, old client
#   > Old core, new client
# Migration (set up SQLite, then --select-backend PostgreSQL)
# > SQLite -> Postgres, new core, new client
# Upgrading existing (set up a copy from 'master', then build your branch)
# > SQLite
#   > Old -> new core
#   > Old monolithic -> new monolithic
# > Postgres
#   > Old -> new core
#
# [Windows]
# Fresh configuration (reset the database and settings)
# > SQLite
#   > New core, new client
#   > New core, old client
#   > Old core, new client
#   > New monolithic (combined core/client build)
# Upgrading existing (set up a copy from 'master', then build your branch)
# > SQLite
#   > Old -> new core
#   > Old monolithic -> new monolithic
# (If someone figures out how Postgres runs on Windows with Quassel, please
#  update this comment)
#
# Yes, this looks excessive, and maybe it is.  But it's easy to overlook
# some minor typo that breaks the client/core for a certain configuration.
# People may get unhappy and rioting might happen in the streets.  And we don't
# want that, do we?
#
# Thank you for reading this guide and good luck with your changes!

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
