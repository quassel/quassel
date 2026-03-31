/***************************************************************************
 *   Copyright (C) 2005-2022 by the Quassel Project                        *
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

#include "integrationtestsupport.h"

#include <memory>
#include <vector>

#include <QByteArray>
#include <QTemporaryDir>

#include "client.h"
#include "core.h"
#include "coreconnection.h"
#include "internalpeer.h"
#include "messagemodel.h"

namespace {

class TestMessageModelItem : public MessageModelItem
{
public:
    explicit TestMessageModelItem(const Message& message)
        : _message(message)
        , _bufferId(message.bufferId())
    {}

    const Message& message() const override { return _message; }
    const QDateTime& timestamp() const override { return _message.timestamp(); }
    const MsgId& msgId() const override { return _message.msgId(); }
    const BufferId& bufferId() const override { return _bufferId; }
    void setBufferId(BufferId bufferId) override { _bufferId = bufferId; }
    Message::Type msgType() const override { return _message.type(); }
    Message::Flags msgFlags() const override { return _message.flags(); }

private:
    Message _message;
    BufferId _bufferId;
};

class TestMessageModel : public MessageModel
{
public:
    explicit TestMessageModel(QObject* parent = nullptr)
        : MessageModel(parent)
    {}

protected:
    int messageCount() const override { return static_cast<int>(_messages.size()); }
    bool messagesIsEmpty() const override { return _messages.empty(); }
    const MessageModelItem* messageItemAt(int i) const override { return _messages.at(i).get(); }
    MessageModelItem* messageItemAt(int i) override { return _messages[i].get(); }
    const MessageModelItem* firstMessageItem() const override { return _messages.front().get(); }
    MessageModelItem* firstMessageItem() override { return _messages.front().get(); }
    const MessageModelItem* lastMessageItem() const override { return _messages.back().get(); }
    MessageModelItem* lastMessageItem() override { return _messages.back().get(); }

    void insertMessage__(int pos, const Message& message) override
    {
        _messages.insert(_messages.begin() + pos, std::make_unique<TestMessageModelItem>(message));
    }

    void insertMessages__(int pos, const QList<Message>& messages) override
    {
        int index = pos;
        for (const Message& message : messages) {
            _messages.insert(_messages.begin() + index++, std::make_unique<TestMessageModelItem>(message));
        }
    }

    void removeMessageAt(int i) override
    {
        _messages.erase(_messages.begin() + i);
    }

    void removeAllMessages() override
    {
        _messages.clear();
    }

    Message takeMessageAt(int i) override
    {
        auto messageItem = std::move(_messages[i]);
        _messages.erase(_messages.begin() + i);
        return messageItem->message();
    }

private:
    std::vector<std::unique_ptr<TestMessageModelItem>> _messages;
};

QTemporaryDir* integrationTempDir()
{
    static auto* tempDir = new QTemporaryDir();
    Q_ASSERT(tempDir->isValid());
    return tempDir;
}

void initializeEnvironment()
{
    const QByteArray tempPath = integrationTempDir()->path().toUtf8();
    qputenv("HOME", tempPath);
    qputenv("XDG_CONFIG_HOME", tempPath);
    qputenv("XDG_DATA_HOME", tempPath);
    qputenv("XDG_CACHE_HOME", tempPath);
    qputenv("XDG_STATE_HOME", tempPath);
}

}  // namespace

namespace test {

TestMessageProcessor::TestMessageProcessor(QObject* parent)
    : AbstractMessageProcessor(parent)
{}

void TestMessageProcessor::process(Message& msg)
{
    preProcess(msg);
}

void TestMessageProcessor::process(QList<Message>& msgs)
{
    for (Message& msg : msgs) {
        preProcess(msg);
    }
}

MessageModel* TestUi::createMessageModel(QObject* parent)
{
    return new TestMessageModel(parent);
}

AbstractMessageProcessor* TestUi::createMessageProcessor(QObject* parent)
{
    return new TestMessageProcessor(parent);
}

Quassel* QuasselTestSupport::ensureInitialized(Quassel::RunMode runMode)
{
    static Quassel* quassel = nullptr;
    static Quassel::RunMode activeRunMode = runMode;

    if (quassel) {
        Q_ASSERT(activeRunMode == runMode);
        return quassel;
    }

    initializeEnvironment();

    quassel = new Quassel();
    Quassel::setupBuildInfo();
    quassel->init(runMode);
    activeRunMode = runMode;
    return quassel;
}

QString QuasselTestSupport::configDirPath()
{
    return ensureInitialized()->configDirPath();
}

std::unique_ptr<TestUi> QuasselTestSupport::createTestUi()
{
    ensureInitialized();
    return std::make_unique<TestUi>();
}

InternalCoreConnectionBridge::InternalCoreConnectionBridge(Core* core, QObject* parent)
    : QObject(parent)
    , _core(core)
{
    Q_ASSERT(_core);

    connect(Client::coreConnection(), &CoreConnection::connectToInternalCore, _core, &Core::connectInternalPeer);
    connect(_core, &Core::sessionStateReceived, Client::coreConnection(), &CoreConnection::internalSessionStateReceived);
}

}  // namespace test
