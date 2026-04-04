/***************************************************************************
 *   Copyright (C) 2005-2026 by the Quassel Project                        *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) version 3.                                           *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "testglobal.h"

#include <QUuid>

#include <QCoreApplication>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

#include "core.h"
#include "integrationtestsupport.h"

namespace {

struct PostgreSqlTestConfig
{
    QString userName;
    QString password;
    QString hostName{QStringLiteral("localhost")};
    int port{5432};
    QString maintenanceDatabase{QStringLiteral("postgres")};
    QString databasePrefix{QStringLiteral("quassel_it_")};

    QVariantMap storageProperties(const QString& databaseName) const
    {
        QVariantMap properties;
        properties["Username"] = userName;
        properties["Password"] = password;
        properties["Hostname"] = hostName;
        properties["Port"] = port;
        properties["Database"] = databaseName;
        return properties;
    }
};

QString sanitizeIdentifier(QString value)
{
    value = value.toLower().replace(QRegularExpression(QStringLiteral("[^a-z0-9_]")), QStringLiteral("_"));
    value.remove(QRegularExpression(QStringLiteral("^[^a-z_]+")));
    if (value.isEmpty()) {
        value = QStringLiteral("quassel_it_");
    }
    if (!value.endsWith(QLatin1Char('_'))) {
        value.append(QLatin1Char('_'));
    }
    return value;
}

bool loadConfig(PostgreSqlTestConfig& config, QString& reason)
{
    const auto environment = QProcessEnvironment::systemEnvironment();
    if (!environment.contains(QStringLiteral("QUASSEL_TEST_PGSQL_USERNAME"))) {
        reason = QStringLiteral("Set QUASSEL_TEST_PGSQL_USERNAME to enable PostgreSQL integration coverage.");
        return false;
    }

    config.userName = environment.value(QStringLiteral("QUASSEL_TEST_PGSQL_USERNAME"));
    config.password = environment.value(QStringLiteral("QUASSEL_TEST_PGSQL_PASSWORD"));
    config.hostName = environment.value(QStringLiteral("QUASSEL_TEST_PGSQL_HOSTNAME"), config.hostName);
    config.port = environment.value(QStringLiteral("QUASSEL_TEST_PGSQL_PORT"), QString::number(config.port)).toInt();
    config.maintenanceDatabase = environment.value(QStringLiteral("QUASSEL_TEST_PGSQL_MAINTENANCE_DATABASE"), config.maintenanceDatabase);
    config.databasePrefix = sanitizeIdentifier(environment.value(QStringLiteral("QUASSEL_TEST_PGSQL_DATABASE_PREFIX"), config.databasePrefix));
    return true;
}

class TemporaryPostgreSqlDatabase
{
public:
    explicit TemporaryPostgreSqlDatabase(const PostgreSqlTestConfig& config)
        : _config(config)
        , _connectionName(QStringLiteral("quassel_test_pg_admin_%1").arg(QUuid::createUuid().toString(QUuid::Id128)))
        , _databaseName(QStringLiteral("%1%2").arg(_config.databasePrefix, QUuid::createUuid().toString(QUuid::Id128)))
    {}

    ~TemporaryPostgreSqlDatabase()
    {
        if (_adminDb.isValid()) {
            if (_adminDb.isOpen()) {
                dropDatabase();
                _adminDb.close();
            }
            const QString connectionName = _connectionName;
            _adminDb = {};
            QSqlDatabase::removeDatabase(connectionName);
        }
    }

    bool create(QString& error)
    {
        _adminDb = QSqlDatabase::addDatabase(QStringLiteral("QPSQL"), _connectionName);
        _adminDb.setHostName(_config.hostName);
        _adminDb.setPort(_config.port);
        _adminDb.setDatabaseName(_config.maintenanceDatabase);
        _adminDb.setUserName(_config.userName);
        _adminDb.setPassword(_config.password);

        if (!_adminDb.open()) {
            error = _adminDb.lastError().text();
            return false;
        }

        QSqlQuery createQuery(_adminDb);
        const QString createStatement = QStringLiteral("CREATE DATABASE %1").arg(_databaseName);
        if (!createQuery.exec(createStatement)) {
            error = createQuery.lastError().text();
            return false;
        }

        return true;
    }

    QString databaseName() const
    {
        return _databaseName;
    }

private:
    void dropDatabase()
    {
        QSqlQuery terminateQuery(_adminDb);
        terminateQuery.prepare(QStringLiteral(
            "SELECT pg_terminate_backend(pid) FROM pg_stat_activity WHERE datname = :database AND pid <> pg_backend_pid()"));
        terminateQuery.bindValue(QStringLiteral(":database"), _databaseName);
        terminateQuery.exec();

        QSqlQuery dropQuery(_adminDb);
        dropQuery.exec(QStringLiteral("DROP DATABASE IF EXISTS %1").arg(_databaseName));
    }

    PostgreSqlTestConfig _config;
    QString _connectionName;
    QString _databaseName;
    QSqlDatabase _adminDb;
};

void skipPostgreSqlTest(const QString& reason)
{
    qInfo() << "Skipping PostgreSQL integration test:" << reason;
}

}  // namespace

class PostgreSqlStorageTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        test::QuasselTestSupport::ensureInitialized(Quassel::Monolithic);
    }
};

TEST_F(PostgreSqlStorageTest, setupInitAndValidateUser)
{
    if (!QSqlDatabase::isDriverAvailable(QStringLiteral("QPSQL"))) {
        skipPostgreSqlTest(QStringLiteral("Qt was built without the QPSQL driver plugin."));
        return;
    }

    PostgreSqlTestConfig config;
    QString skipReason;
    if (!loadConfig(config, skipReason)) {
        skipPostgreSqlTest(skipReason);
        return;
    }

    TemporaryPostgreSqlDatabase database(config);
    QString databaseError;
    ASSERT_TRUE(database.create(databaseError)) << qPrintable(databaseError);

    {
        Core core;
        core.init();

        const QString userName = QStringLiteral("integration-%1").arg(QUuid::createUuid().toString(QUuid::Id128));
        const QString password = QStringLiteral("s3cret-password");
        const QString setupError = Core::setup(userName,
                                               password,
                                               QStringLiteral("PostgreSQL"),
                                               config.storageProperties(database.databaseName()),
                                               QStringLiteral("Database"),
                                               {});

        ASSERT_TRUE(setupError.isEmpty()) << qPrintable(setupError);

        const UserId userId = Core::getUserId(userName);
        ASSERT_TRUE(userId.isValid());
        EXPECT_EQ(userId, Core::validateUser(userName, password));

        Core::setUserSetting(userId, "integration/test-setting", QStringLiteral("stored-value"));
        EXPECT_EQ(QStringLiteral("stored-value"),
                  Core::getUserSetting(userId, "integration/test-setting").toString());
    }

    QCoreApplication::processEvents();
}
