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

#include <QSignalSpy>

#include "client.h"
#include "core.h"
#include "coreconnection.h"
#include "integrationtestsupport.h"

class HeadlessSyncTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        test::QuasselTestSupport::ensureInitialized(Quassel::Monolithic);
    }
};

TEST_F(HeadlessSyncTest, internalCoreConnectionSynchronizesClient)
{
    Client client(test::QuasselTestSupport::createTestUi());
    Core core;
    core.init();

    test::InternalCoreConnectionBridge bridge(&core);

    QSignalSpy connectedSpy(&client, &Client::connected);
    QSignalSpy errorSpy(Client::coreConnection(), &CoreConnection::connectionError);

    ASSERT_TRUE(Client::coreConnection()->connectToCore());

    if (!Client::isConnected()) {
        ASSERT_TRUE(connectedSpy.wait(5000));
    }

    EXPECT_TRUE(Client::isConnected());
    EXPECT_TRUE(Client::internalCore());
    EXPECT_EQ(CoreConnection::Synchronized, Client::coreConnection()->state());
    EXPECT_TRUE(Client::coreConnection()->isConnected());
    EXPECT_TRUE(Client::coreConnection()->currentAccount().isInternal());
    EXPECT_NE(nullptr, Client::bufferSyncer());
    EXPECT_TRUE(errorSpy.isEmpty());
}
