quassel_add_test(SqliteStorageTest LIBRARIES Quassel::Core Quassel::Test::Util)
quassel_add_test(HeadlessSyncTest LIBRARIES Quassel::Client Quassel::Core Quassel::Test::Util)
quassel_add_test(MonolithicLifecycleTest LIBRARIES Quassel::Client Quassel::Core Quassel::Test::Util)
quassel_add_test(PostgreSqlStorageTest LIBRARIES Quassel::Core Quassel::Test::Util)
