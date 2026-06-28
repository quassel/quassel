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

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QMetaObject>
#include <QPointer>
#include <QSignalSpy>
#include <QThread>

#include "client.h"
#include "core.h"
#include "coreconnection.h"
#include "integrationtestsupport.h"

namespace {

class ThreadedCoreHarness
{
public:
    Core* start()
    {
        Q_ASSERT(!_core);

        _core = new Core{};
        _core->moveToThread(&_thread);
        QObject::connect(&_thread, &QThread::started, _core, &Core::initAsync);
        QObject::connect(&_thread, &QThread::finished, _core, &QObject::deleteLater);
        _thread.start();
        return _core;
    }

    Core* core() const
    {
        return _core;
    }

    bool shutdownAndWait(int timeout = 15000)
    {
        if (!_core || !_thread.isRunning()) {
            return true;
        }

        QSignalSpy shutdownSpy(_core, &Core::shutdownComplete);
        QObject::connect(_core, &Core::shutdownComplete, &_thread, &QThread::quit, Qt::UniqueConnection);
        QMetaObject::invokeMethod(
            _core,
            [core = _core]() {
                if (core) {
                    core->shutdown();
                }
            },
            Qt::QueuedConnection);

        QElapsedTimer timer;
        timer.start();

        bool didShutdown = false;
        while (timer.elapsed() < timeout && !didShutdown) {
            didShutdown = shutdownSpy.wait(1000);
            QCoreApplication::processEvents();
        }

        const int remaining = std::max(0, timeout - static_cast<int>(timer.elapsed()));
        const bool threadStopped = _thread.wait(remaining);
        QCoreApplication::processEvents();
        return didShutdown && threadStopped;
    }

    ~ThreadedCoreHarness()
    {
        shutdownAndWait();
    }

private:
    QThread _thread;
    QPointer<Core> _core;
};

}  // namespace

class MonolithicLifecycleTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        test::QuasselTestSupport::ensureInitialized(Quassel::Monolithic);
    }
};

TEST_F(MonolithicLifecycleTest, threadedInternalCoreStartsSynchronizesAndShutsDown)
{
    ThreadedCoreHarness coreHarness;
    Core* core = coreHarness.start();
    ASSERT_TRUE(core);

    QSignalSpy coreExitSpy(core, &Core::exitRequested);

    {
        Client client(test::QuasselTestSupport::createTestUi());
        test::InternalCoreConnectionBridge bridge(core);

        QSignalSpy connectedSpy(&client, &Client::connected);
        QSignalSpy errorSpy(Client::coreConnection(), &CoreConnection::connectionError);

        ASSERT_TRUE(Client::coreConnection()->connectToCore());

        if (!Client::isConnected()) {
            ASSERT_TRUE(connectedSpy.wait(5000));
        }

        EXPECT_TRUE(Client::isConnected());
        EXPECT_TRUE(Client::internalCore());
        EXPECT_EQ(CoreConnection::Synchronized, Client::coreConnection()->state());
        EXPECT_NE(nullptr, Client::bufferSyncer());
        EXPECT_TRUE(errorSpy.isEmpty());
    }

    QCoreApplication::sendPostedEvents(nullptr, 0);
    QCoreApplication::processEvents();

    EXPECT_TRUE(coreExitSpy.isEmpty());
    EXPECT_TRUE(coreHarness.shutdownAndWait());
}
