# Database schema management in Quassel IRC

## Introduction

Quassel IRC supports two database backends for storing chat history, user
sessions, etc:

* [PostgreSQL][postgres-home], in the [`PostgreSQL`](PostgreSQL/) folder
* [SQLite][sqlite-home], in the [`SQLite`](SQLite/) folder

For the most part, Quassel strives to have consistency between the two
backends, offering the same user experience and similar `.sql` query files,
though there are times when one database supports features that another does
not.

All of the current schema version queries are stored at the top level of the
backend subdirectory, with schema upgrade queries stored in
`[backend]/version/##` folders.

At compile time, the build system generates and reads a Qt resource file to
know which queries to include.  For past Quassel contributors, this replaces
the classic `sql.qrc` file and `updateSQLResource.sh` script.

## Managing the database outside of Quassel

Whenever possible, it's recommended to use Quassel itself to make changes, such
as deleting chats or changing user passwords.

However, some tasks aren't yet possible in Quassel, and you should
[read the documentation on managing the database][docs-wiki-db-manage] if the
need arises.

## Making changes to the database
### Changes to existing queries, no new/moved files

If you're only modifying existing queries in a way that does **not** require
any schema changes (*e.g. `ALTER TABLE`, `CREATE TABLE`, `CREATE INDEX`
statements*), you can just modify the `.sql` files in the appropriate backends.

However, any database schema changes must fulfill a number of requirements.

### New queries, schema changes, etc

> One does not simply 'upgrade the schema'...

When changing Quassel's database schema, you need to follow several steps to
handle all cases (*upgrade, Postgres migration, etc*).  Some of these steps may
not apply to all schema upgrades.

1.  **Run [`upgradeSchema.sh`][file-sh-upgradeschema] script on *both*
PostgreSQL and SQLite directories**

Make sure you're on the Git branch you want to modify

*Example:*
```sh
./upgradeSchema.sh "PostgreSQL"
./upgradeSchema.sh "SQLite"
```

Or, you can manually create new `[backend]/version/##` folders for both
`PostgreSQL` and `SQLite`.  Pick the next higher number from the largest
version number for each (*the exact version numbers will usually differ; that's
fine*).

2.  **Modify queries and setup scripts to handle your change**

The specifics depend on your change; in general, you'll want to modify any
query files that select from or insert to a modified table.  Then, modify the
`setup_###_[...].sql` files to include your changes on new database installs.

> *Example: modifying the `ircserver` table to add column `test`*
>
> Modify all query/setup `.sql` files that touch the `ircserver` table for
> *both* `PostgreSQL` and `SQLite`.

3.  **Create upgrade scripts for *both* PostgreSQL and SQLite**

These should go in the newest `[backend]/version/##` folders that were created
in Step 1.

Outside of special circumstances, do **not** modify the files in lower-numbered
version folders.  Existing Quassel cores have already run those statements and
will not run them if changed.

> *Example: modifying the `ircserver` table to add column `test`*
>
> Add the file `upgrade_000_alter_ircserver_add_test.sql` with contents:
>
> ```sql
> ALTER TABLE ircserver
> ADD COLUMN test [additional column-specific details]
> ```

4.  **Update the pair of migration scripts for moving from SQLite to
PostgreSQL**

For any table changes, you'll need to update the relevant
`SQLite/migrate_read_[table].sql` to read the existing data, and the
`PostgreSQL/migrate_write_[table].sql` to insert this data.

> *Example: modifying the `ircserver` table to add column `test`*
>
> Modify `SQLite/migrate_read_ircserver.sql` to select from new column
>
> Modify `PostgreSQL/migrate_write_ircserver.sql` to insert to new column

5.  **Update the migration logic in
[`src/core/abstractsqlstorage.cpp`][file-cpp-abstract], and the storage
backends [`postgresqlstorage.cpp`][file-cpp-postgres] and
[`sqlitestorage.cpp`][file-cpp-sqlite]**

Make sure to read the data from SQLite in the right types and order, then write
it to PostgreSQL also with the right types and order.  SQLite and PostgreSQL
may use different database representations of certain types.

> *Example: modifying the `ircserver` table to add column `test`*
>
> Modify struct `IrcServerMO` in [`abstractsqlstorage.h`][file-h-abstract],
> adding an entry for `test` of the appropriate data-type.
>
> Modify `SqliteMigrationReader::readMo(IrcServerMO &ircserver)` in
> [`sqlitestorage.cpp`][file-cpp-sqlite] to read from the new column and store
> it in the migration object.  You may need to convert from SQLite's looser
> types.
>
> Modify `PostgreSqlMigrationWriter::writeMo(const IrcServerMO &ircserver)` in
> [`postgresqlstorage.cpp`][file-cpp-postgres] to write to the new column from
> the data in the migration object.

6.  **Update any affected queries in storage backends
[`postgresqlstorage.cpp`][file-cpp-postgres] and
[`sqlitestorage.cpp`][file-cpp-sqlite], and any related synchronized
`src/common` classes.**

> *Example: modifying the `ircserver` table to add column `test`*
>
> Update `network.h` to add new column to Server structure:
> ```diff
>    QString proxyPass;                                    // Existing code
> +  Typename test;                                        // New column 'test'
>    [...]
> -  Server() : port(6667), ..., proxyPort(8080) {}
> +  Server() : port(6667), ..., proxyPort(8080), test("defaultValue") {}
> ```
>
> Modify reading data in `[PostgreSql/Sqlite]Storage::networks(...)`:
> ```diff
>    server.proxyPass = serversQuery.value(10).toString(); // Existing code
> +  server.test = serversQuery.value(11).toType();        // New column 'test'
>    servers << server;                                    // Existing code
> ```
>
> Modify writing data in `[PostgreSql/Sqlite]Storage::bindServerInfo(...)`:
> ```diff
>    query.bindValue(":proxypass", server.proxyPass);      // Existing code
> +  query.bindValue(":test", server.test);                // New column 'test'
> ```

7.  **If protocol changed (*add a setting, etc*), add a new `Feature` flag**

Newer clients need to detect when they're on an older core to disable the
feature.  Use the `Feature` enum in [`quassel.h`][file-h-quassel].

In client-side code, test for a feature with...

```cpp
if (Client::isCoreFeatureEnabled(Quassel::Feature::FeatureName)) { ... }
```

8.  **Test everything!  Upgrade, migrate, new setups, new client/old core,
old client/new core, etc.**

More specifically, you should try the following combinations, especially if you
change the protocol.  Check if any data or settings get lost or corrupted, or
if anything unusual shows up in the log.  Restart the Quassel core and client,
to check that data persists.

---

**Linux/macOS**

*Fresh configuration (initialize the database and settings from scratch)*

Test conditions  | Result  | Remarks
-----------------|---------|-------------
SQLite, new core, new client | ❓ **TODO** |
SQLite, new core, old client | ❓ **TODO** |
SQLite, old core, new client | ❓ **TODO** |
SQLite, new monolithic | ❓ **TODO** |
Postgres, new core, new client | ❓ **TODO** |
Postgres, new core, old client | ❓ **TODO** |
Postgres, old core, new client | ❓ **TODO** |

*Migration (set up SQLite, then `--select-backend PostgreSQL`)*

Test conditions  | Result  | Remarks
-----------------|---------|-------------
SQLite → Postgres, new core, new client | ❓ **TODO** |

*Upgrading existing (set up a copy from `master`, then upgrade to your branch)*

Test conditions  | Result  | Remarks
-----------------|---------|-------------
SQLite, old → new core | ❓ **TODO** |
SQLite, old monolithic → new monolithic | ❓ **TODO** |
Postgres, old → new core | ❓ **TODO** |

**Windows**

*Fresh configuration (initialize the database and settings from scratch)*

Test conditions  | Result  | Remarks
-----------------|---------|-------------
SQLite, new core, new client | ❓ **TODO** |
SQLite, new core, old client | ❓ **TODO** |
SQLite, old core, new client | ❓ **TODO** |
SQLite, new monolithic | ❓ **TODO** |

*Upgrading existing (set up a copy from `master`, then upgrade to your branch)*

Test conditions  | Result  | Remarks
-----------------|---------|-------------
SQLite, old → new core | ❓ **TODO** |
SQLite, old monolithic → new monolithic | ❓ **TODO** |

*If someone figures out how Postgres runs on Windows with Quassel, please
update this...*

---

Yes, this looks excessive, and maybe it is.  But it's easy to overlook
some minor typo that breaks the client/core for a certain configuration.
People may get unhappy and rioting might happen in the streets.  And we don't
want that, do we?

Thank you for reading this guide and good luck with your changes!

[postgres-home]: https://www.postgresql.org/
[sqlite-home]: https://sqlite.org/
[docs-wiki-db-manage]: https://bugs.quassel-irc.org/projects/quassel-irc/wiki/Database_Management
[file-cpp-abstract]: ../abstractsqlstorage.cpp
[file-h-abstract]: ../abstractsqlstorage.h
[file-cpp-postgres]: ../postgresqlstorage.cpp
[file-cpp-sqlite]: ../sqlitestorage.cpp
[file-sh-upgradeschema]: upgradeSchema.sh
[file-h-quassel]: ../../common/quassel.h
