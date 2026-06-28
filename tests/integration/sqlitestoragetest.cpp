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

#include "core.h"
#include "integrationtestsupport.h"

class SqliteStorageTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        test::QuasselTestSupport::ensureInitialized(Quassel::Monolithic);
    }
};

TEST_F(SqliteStorageTest, setupInitAndValidateUser)
{
    Core core;
    core.init();

    const QString userName = QStringLiteral("integration-%1").arg(QUuid::createUuid().toString(QUuid::Id128));
    const QString password = QStringLiteral("s3cret-password");
    const QString setupError = Core::setup(userName, password, "SQLite", {}, "Database", {});

    ASSERT_TRUE(setupError.isEmpty()) << qPrintable(setupError);

    const UserId userId = Core::getUserId(userName);
    ASSERT_TRUE(userId.isValid());
    EXPECT_EQ(userId, Core::validateUser(userName, password));

    Core::setUserSetting(userId, "integration/test-setting", QStringLiteral("stored-value"));
    EXPECT_EQ(QStringLiteral("stored-value"),
              Core::getUserSetting(userId, "integration/test-setting").toString());
}
