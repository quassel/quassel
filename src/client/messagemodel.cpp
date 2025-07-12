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

#include "messagemodel.h"

#include <algorithm>

#include <QEvent>
#include <QTimeZone>  // Added for QTimeZone

#include "backlogsettings.h"
#include "client.h"
#include "clientbacklogmanager.h"
#include "message.h"
#include "networkmodel.h"

class ProcessBufferEvent : public QEvent
{
public:
    inline ProcessBufferEvent()
        : QEvent(QEvent::User)
    {
    }
};

MessageModel::MessageModel(QObject* parent)
    : QAbstractItemModel(parent)
{
    QDateTime now = QDateTime::currentDateTime();
    now.setTimeZone(QTimeZone::UTC);             // Replaced setTimeSpec(Qt::UTC)
    _nextDayChange.setTimeZone(QTimeZone::UTC);  // Replaced setTimeSpec(Qt::UTC)
    _nextDayChange.setMSecsSinceEpoch(((now.toMSecsSinceEpoch() / DAY_IN_MSECS) + 1) * DAY_IN_MSECS);
    _nextDayChange.setTimeZone(QTimeZone::systemTimeZone());  // Replaced setTimeSpec(Qt::LocalTime)
    _dayChangeTimer.setInterval(QDateTime::currentDateTime().secsTo(_nextDayChange) * 1000);
    _dayChangeTimer.start();
    connect(&_dayChangeTimer, &QTimer::timeout, this, &MessageModel::changeOfDay);
}

QVariant MessageModel::data(const QModelIndex& index, int role) const
{
    int row = index.row();
    int column = index.column();
    if (row < 0 || row >= messageCount() || column < 0)
        return QVariant();

    if (role == ColumnTypeRole)
        return column;

    return messageItemAt(row)->data(index.column(), role);
}

bool MessageModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    int row = index.row();
    if (row < 0 || row >= messageCount())
        return false;

    if (messageItemAt(row)->setData(index.column(), value, role)) {
        emit dataChanged(index, index);
        return true;
    }
    return false;
}

bool MessageModel::insertMessage(const Message& msg, bool fakeMsg)
{
    MsgId id = msg.msgId();
    int idx = indexForId(id);
    if (!fakeMsg && idx < messageCount()) {  // check for duplicate
        if (messageItemAt(idx)->msgId() == id)
            return false;
    }

    insertMessageGroup(QList<Message>() << msg);
    return true;
}

void MessageModel::insertMessages(const QList<Message>& msglist)
{
    if (msglist.isEmpty())
        return;

    if (_messageBuffer.isEmpty()) {
        int processedMsgs = insertMessagesGracefully(msglist);
        int remainingMsgs = msglist.count() - processedMsgs;
        if (remainingMsgs > 0) {
            if (msglist.first().msgId() < msglist.last().msgId()) {
                // in Order - we have just successfully processed "processedMsg" messages from the end of the list
                _messageBuffer = msglist.mid(0, remainingMsgs);
            }
            else {
                _messageBuffer = msglist.mid(processedMsgs);
            }
            std::sort(_messageBuffer.begin(), _messageBuffer.end());
            QCoreApplication::postEvent(this, new ProcessBufferEvent());
        }
    }
    else {
        _messageBuffer << msglist;
        std::sort(_messageBuffer.begin(), _messageBuffer.end());
    }
}

void MessageModel::insertMessageGroup(const QList<Message>& msglist)
{
    Q_ASSERT(!msglist.isEmpty());
    int start = indexForId(msglist.first().msgId());
    int end = start + msglist.count() - 1;
    Message dayChangeMsg;

    if (start > 0) {
        // check if the preceding msg is a daychange message and if so if
        // we have to drop or relocate it at the end of this chunk
        int prevIdx = start - 1;
        if (messageItemAt(prevIdx)->msgType() == Message::DayChange && messageItemAt(prevIdx)->timestamp() > msglist.at(0).timestamp()) {
            beginRemoveRows(QModelIndex(), prevIdx, prevIdx);
            Message oldDayChangeMsg = takeMessageAt(prevIdx);
            if (msglist.last().timestamp() < oldDayChangeMsg.timestamp()) {
                // we have to reinsert it with a changed msgId
                dayChangeMsg = oldDayChangeMsg;
                dayChangeMsg.setMsgId(msglist.last().msgId());
            }
            endRemoveRows();

            start--;
            end--;
        }
    }

    if (!dayChangeMsg.isValid() && start < messageCount()) {
        Q_ASSERT(messageItemAt(start)->msgType() != Message::DayChange);
        QDateTime nextTs = messageItemAt(start)->timestamp();
        QDateTime prevTs = msglist.last().timestamp();
        nextTs.setTimeZone(QTimeZone::UTC);  // Replaced setTimeSpec(Qt::UTC)
        prevTs.setTimeZone(QTimeZone::UTC);  // Replaced setTimeSpec(Qt::UTC)
        qint64 nextDay = nextTs.toMSecsSinceEpoch() / DAY_IN_MSECS;
        qint64 prevDay = prevTs.toMSecsSinceEpoch() / DAY_IN_MSECS;
        if (nextDay != prevDay) {
            nextTs.setMSecsSinceEpoch(nextDay * DAY_IN_MSECS);
            nextTs.setTimeZone(QTimeZone::systemTimeZone());  // Replaced setTimeSpec(Qt::LocalTime)
            dayChangeMsg = Message::ChangeOfDay(nextTs);
            dayChangeMsg.setMsgId(msglist.last().msgId());
        }
    }

    if (dayChangeMsg.isValid())
        end++;

    Q_ASSERT(start == 0 || messageItemAt(start - 1)->msgId() < msglist.first().msgId());
    Q_ASSERT(start == messageCount() || messageItemAt(start)->msgId() > msglist.last().msgId());
    beginInsertRows(QModelIndex(), start, end);
    insertMessages__(start, msglist);
    if (dayChangeMsg.isValid())
        insertMessage__(start + msglist.count(), dayChangeMsg);
    endInsertRows();

    Q_ASSERT(start == end || messageItemAt(start)->msgId() != messageItemAt(end)->msgId()
             || messageItemAt(end)->msgType() == Message::DayChange);
    Q_ASSERT(start == 0 || messageItemAt(start - 1)->msgId() < messageItemAt(start)->msgId());
    Q_ASSERT(end + 1 == messageCount() || messageItemAt(end)->msgId() < messageItemAt(end + 1)->msgId());
}

int MessageModel::insertMessagesGracefully(const QList<Message>& msglist)
{
    bool inOrder = (msglist.first().msgId() < msglist.last().msgId());
    QList<Message> grouplist;
    MsgId minId;
    MsgId dupeId;
    int processedMsgs = 1;
    int idx;
    bool fastForward = false;
    QList<Message>::const_iterator iter;
    if (inOrder) {
        iter = msglist.constEnd();
        --iter;
    }
    else {
        iter = msglist.constBegin();
    }

    idx = indexForId((*iter).msgId());
    if (idx < messageCount())
        dupeId = messageItemAt(idx)->msgId();

    if (idx - 1 >= 0)
        minId = messageItemAt(idx - 1)->msgId();
    else
        fastForward = true;

    if ((*iter).msgId() != dupeId) {
        grouplist << *iter;
        dupeId = (*iter).msgId();
    }

    if (!inOrder)
        ++iter;

    if (inOrder) {
        while (iter != msglist.constBegin()) {
            --iter;

            if (!fastForward && (*iter).msgId() <= minId)
                break;
            processedMsgs++;

            if (grouplist.isEmpty()) {
                idx = indexForId((*iter).msgId());
                if (idx >= 0 && !messagesIsEmpty())
                    dupeId = messageItemAt(idx)->msgId();
            }
            if ((*iter).msgId() != dupeId) {
                if (!grouplist.isEmpty()) {
                    QDateTime nextTs = grouplist.value(0).timestamp();
                    QDateTime prevTs = (*iter).timestamp();
                    nextTs.setTimeZone(QTimeZone::UTC);  // Replaced setTimeSpec(Qt::UTC)
                    prevTs.setTimeZone(QTimeZone::UTC);  // Replaced setTimeSpec(Qt::UTC)
                    qint64 nextDay = nextTs.toMSecsSinceEpoch() / DAY_IN_MSECS;
                    qint64 prevDay = prevTs.toMSecsSinceEpoch() / DAY_IN_MSECS;
                    if (nextDay != prevDay) {
                        nextTs.setMSecsSinceEpoch(nextDay * DAY_IN_MSECS);
                        nextTs.setTimeZone(QTimeZone::systemTimeZone());  // Replaced setTimeSpec(Qt::LocalTime)
                        Message dayChangeMsg = Message::ChangeOfDay(nextTs);
                        dayChangeMsg.setMsgId((*iter).msgId());
                        grouplist.prepend(dayChangeMsg);
                    }
                }
                dupeId = (*iter).msgId();
                grouplist.prepend(*iter);
            }
        }
    }
    else {
        while (iter != msglist.constEnd()) {
            if (!fastForward && (*iter).msgId() <= minId)
                break;
            processedMsgs++;

            if (grouplist.isEmpty()) {
                idx = indexForId((*iter).msgId());
                if (idx >= 0 && !messagesIsEmpty())
                    dupeId = messageItemAt(idx)->msgId();
            }
            if ((*iter).msgId() != dupeId) {
                if (!grouplist.isEmpty()) {
                    QDateTime nextTs = grouplist.value(0).timestamp();
                    QDateTime prevTs = (*iter).timestamp();
                    nextTs.setTimeZone(QTimeZone::UTC);  // Replaced setTimeSpec(Qt::UTC)
                    prevTs.setTimeZone(QTimeZone::UTC);  // Replaced setTimeSpec(Qt::UTC)
                    qint64 nextDay = nextTs.toMSecsSinceEpoch() / DAY_IN_MSECS;
                    qint64 prevDay = prevTs.toMSecsSinceEpoch() / DAY_IN_MSECS;
                    if (nextDay != prevDay) {
                        nextTs.setMSecsSinceEpoch(nextDay * DAY_IN_MSECS);
                        nextTs.setTimeZone(QTimeZone::systemTimeZone());  // Replaced setTimeSpec(Qt::LocalTime)
                        Message dayChangeMsg = Message::ChangeOfDay(nextTs);
                        dayChangeMsg.setMsgId((*iter).msgId());
                        grouplist.prepend(dayChangeMsg);
                    }
                }
                dupeId = (*iter).msgId();
                grouplist.prepend(*iter);
            }
            ++iter;
        }
    }

    if (!grouplist.isEmpty())
        insertMessageGroup(grouplist);
    return processedMsgs;
}

void MessageModel::customEvent(QEvent* event)
{
    if (event->type() != QEvent::User)
        return;

    event->accept();

    if (_messageBuffer.isEmpty())
        return;

    int processedMsgs = insertMessagesGracefully(_messageBuffer);
    int remainingMsgs = _messageBuffer.count() - processedMsgs;

    QList<Message>::iterator removeStart = _messageBuffer.begin() + remainingMsgs;
    QList<Message>::iterator removeEnd = _messageBuffer.end();
    _messageBuffer.erase(removeStart, removeEnd);
    if (!_messageBuffer.isEmpty())
        QCoreApplication::postEvent(this, new ProcessBufferEvent());
}

void MessageModel::clear()
{
    _messagesWaiting.clear();
    if (rowCount() > 0) {
        beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
        removeAllMessages();
        endRemoveRows();
    }
}

int MessageModel::indexForId(MsgId id)
{
    if (messagesIsEmpty() || id <= messageItemAt(0)->msgId())
        return 0;

    if (id > lastMessageItem()->msgId())
        return messageCount();

    // binary search
    int start = 0;
    int end = messageCount() - 1;
    while (true) {
        if (end - start == 1)
            return end;
        int pivot = (end + start) / 2;
        if (id <= messageItemAt(pivot)->msgId())
            end = pivot;
        else
            start = pivot;
    }
}

void MessageModel::changeOfDay()
{
    _dayChangeTimer.setInterval(DAY_IN_MSECS);
    if (!messagesIsEmpty()) {
        int idx = messageCount();
        while (idx > 0 && messageItemAt(idx - 1)->timestamp() > _nextDayChange) {
            idx--;
        }
        if (idx == 0) {
            qDebug() << Q_FUNC_INFO << "Oldest message with MsgId" << messageItemAt(idx)->msgId() << "has a timestamp of"
                     << messageItemAt(idx)->timestamp() << "which is newer than day change" << _nextDayChange
                     << ".  Skipping inserting a DayChange message.";
        }
        else {
            beginInsertRows(QModelIndex(), idx, idx);
            Message dayChangeMsg = Message::ChangeOfDay(_nextDayChange);
            dayChangeMsg.setMsgId(messageItemAt(idx - 1)->msgId());
            insertMessage__(idx, dayChangeMsg);
            endInsertRows();
        }
    }
    _nextDayChange = _nextDayChange.addMSecs(DAY_IN_MSECS);
}

void MessageModel::insertErrorMessage(BufferInfo bufferInfo, const QString& errorString)
{
    int idx = messageCount();
    beginInsertRows(QModelIndex(), idx, idx);
    Message msg(bufferInfo, Message::Error, errorString);
    if (!messagesIsEmpty())
        msg.setMsgId(messageItemAt(idx - 1)->msgId());
    else
        msg.setMsgId(0);
    insertMessage__(idx, msg);
    endInsertRows();
}

void MessageModel::requestBacklog(BufferId bufferId)
{
    if (_messagesWaiting.contains(bufferId))
        return;

    BacklogSettings backlogSettings;
    int requestCount = backlogSettings.dynamicBacklogAmount();

    MsgId oldestAvailableMsgId{-1};
    for (int i = 0; i < messageCount(); i++) {
        if (messageItemAt(i)->bufferId() == bufferId) {
            oldestAvailableMsgId = messageItemAt(i)->msgId();
            break;
        }
    }

    _messagesWaiting[bufferId] = requestCount;
    Client::backlogManager()->emitMessagesRequested(tr("Requesting %1 messages from backlog for buffer %2:%3")
                                                        .arg(requestCount)
                                                        .arg(Client::networkModel()->networkName(bufferId))
                                                        .arg(Client::networkModel()->bufferName(bufferId)));

    if (oldestAvailableMsgId.isValid()) {
        Client::backlogManager()->requestBacklog(bufferId, -1, oldestAvailableMsgId, requestCount);
    }
    else {
        Client::backlogManager()->requestBacklog(bufferId, -1, -1, requestCount);
    }
}

void MessageModel::messagesReceived(BufferId bufferId, int count)
{
    if (!_messagesWaiting.contains(bufferId))
        return;

    _messagesWaiting[bufferId] -= count;
    if (_messagesWaiting[bufferId] <= 0) {
        _messagesWaiting.remove(bufferId);
        emit finishedBacklogFetch(bufferId);
    }
}

void MessageModel::buffersPermanentlyMerged(BufferId bufferId1, BufferId bufferId2)
{
    for (int i = 0; i < messageCount(); i++) {
        if (messageItemAt(i)->bufferId() == bufferId2) {
            messageItemAt(i)->setBufferId(bufferId1);
            QModelIndex idx = index(i, 0);
            emit dataChanged(idx, idx);
        }
    }
}

// ========================================
//  MessageModelItem
// ========================================
QVariant MessageModelItem::data(int column, int role) const
{
    if (column < MessageModel::TimestampColumn || column > MessageModel::ContentsColumn)
        return QVariant();

    switch (role) {
    case MessageModel::MessageRole:
        return QVariant::fromValue(message());
    case MessageModel::MsgIdRole:
        return QVariant::fromValue(msgId());
    case MessageModel::BufferIdRole:
        return QVariant::fromValue(bufferId());
    case MessageModel::TypeRole:
        return msgType();
    case MessageModel::FlagsRole:
        return (int)msgFlags();
    case MessageModel::TimestampRole:
        return timestamp();
    case MessageModel::RedirectedToRole:
        return QVariant::fromValue(_redirectedTo);
    default:
        return {};
    }
}

bool MessageModelItem::setData(int column, const QVariant& value, int role)
{
    Q_UNUSED(column);

    switch (role) {
    case MessageModel::RedirectedToRole:
        _redirectedTo = value.value<BufferId>();
        return true;
    default:
        return false;
    }
}

bool MessageModelItem::lessThan(const MessageModelItem* m1, const MessageModelItem* m2)
{
    return (*m1) < (*m2);
}

bool MessageModelItem::operator<(const MessageModelItem& other) const
{
    return msgId() < other.msgId();
}

bool MessageModelItem::operator==(const MessageModelItem& other) const
{
    return msgId() == other.msgId();
}

bool MessageModelItem::operator>(const MessageModelItem& other) const
{
    return msgId() > other.msgId();
}

QDebug operator<<(QDebug dbg, const MessageModelItem& msgItem)
{
    dbg.nospace() << qPrintable(QString("MessageModelItem(MsgId:")) << msgItem.msgId() << qPrintable(QString(",")) << msgItem.timestamp()
                  << qPrintable(QString(", Type:")) << msgItem.msgType() << qPrintable(QString(", Flags:")) << msgItem.msgFlags()
                  << qPrintable(QString(")")) << msgItem.data(1, Qt::DisplayRole).toString() << ":"
                  << msgItem.data(2, Qt::DisplayRole).toString();
    return dbg;
}
