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

#pragma once

#include "test-util-export.h"

#include <memory>

#include <QObject>
#include <QString>

#include "abstractmessageprocessor.h"
#include "abstractui.h"
#include "message.h"
#include "quassel.h"

class Core;
class MessageModel;

namespace test {

class TEST_UTIL_EXPORT TestMessageProcessor : public AbstractMessageProcessor
{
    Q_OBJECT

public:
    explicit TestMessageProcessor(QObject* parent = nullptr);

    void reset() override {}
    void process(Message& msg) override;
    void process(QList<Message>& msgs) override;
    void networkRemoved(NetworkId id) override { Q_UNUSED(id); }
};

class TEST_UTIL_EXPORT TestUi : public AbstractUi
{
    Q_OBJECT

public:
    explicit TestUi(QObject* parent = nullptr)
        : AbstractUi(parent)
    {}

    void init() override {}
    MessageModel* createMessageModel(QObject* parent) override;
    AbstractMessageProcessor* createMessageProcessor(QObject* parent) override;
};

class TEST_UTIL_EXPORT QuasselTestSupport
{
public:
    static Quassel* ensureInitialized(Quassel::RunMode runMode = Quassel::Monolithic);
    static QString configDirPath();
    static std::unique_ptr<TestUi> createTestUi();
};

class TEST_UTIL_EXPORT InternalCoreConnectionBridge : public QObject
{
    Q_OBJECT

public:
    explicit InternalCoreConnectionBridge(Core* core, QObject* parent = nullptr);

private:
    Core* _core;
};

}  // namespace test
