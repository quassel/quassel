/***************************************************************************
 *   Copyright (C) 2005-2013 by the Quassel Project                        *
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

#ifndef MESSAGEMODEL_H_
#define MESSAGEMODEL_H_

#include <QAbstractItemModel>
#include <QDateTime>
#include <QTimer>

#include "message.h"
#include "types.h"

class MessageModelItem;
struct MsgId;

class MessageModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum MessageModelRole {
        DisplayRole = Qt::DisplayRole,
        EditRole = Qt::EditRole,
        BackgroundRole = Qt::BackgroundRole,
        MessageRole = Qt::UserRole,
        MsgIdRole,
        BufferIdRole,
        TypeRole,
        FlagsRole,
        TimestampRole,
        FormatRole,
        ColumnTypeRole,
        RedirectedToRole,
        UserRole
    };

    enum ColumnType {
        TimestampColumn, SenderColumn, ContentsColumn, UserColumnType
    };

    MessageModel(QObject *parent);

    inline QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    inline QModelIndex parent(const QModelIndex &) const { return QModelIndex(); }
    inline int rowCount(const QModelIndex &parent = QModelIndex()) const { return parent.isValid() ? 0 : messageCount(); }
    inline int columnCount(const QModelIndex & /*parent*/ = QModelIndex()) const { return 3; }

    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role);

    //virtual Qt::ItemFlags flags(const QModelIndex &index) const;

    bool insertMessage(const Message &, bool fakeMsg = false);
    void insertMessages(const QList<Message> &);

    void clear();

signals:
    void finishedBacklogFetch(BufferId bufferId);

public slots:
    void requestBacklog(BufferId bufferId);
    void messagesReceived(BufferId bufferId, int count);
    void buffersPermanentlyMerged(BufferId bufferId1, BufferId bufferId2);
    void insertErrorMessage(BufferInfo bufferInfo, const QString &errorString);

protected:
//   virtual MessageModelItem *createMessageModelItem(const Message &) = 0;

    virtual int messageCount() const = 0;
    virtual bool messagesIsEmpty() const = 0;
    virtual const MessageModelItem *messageItemAt(int i) const = 0;
    virtual MessageModelItem *messageItemAt(int i) = 0;
    virtual const MessageModelItem *firstMessageItem() const = 0;
    virtual MessageModelItem *firstMessageItem() = 0;
    virtual const MessageModelItem *lastMessageItem() const = 0;
    virtual MessageModelItem *lastMessageItem() = 0;
    virtual void insertMessage__(int pos, const Message &) = 0;
    virtual void insertMessages__(int pos, const QList<Message> &) = 0;
    virtual void removeMessageAt(int i) = 0;
    virtual void removeAllMessages() = 0;
    virtual Message takeMessageAt(int i) = 0;

    virtual void customEvent(QEvent *event);

private slots:
    void changeOfDay();

private:
    void insertMessageGroup(const QList<Message> &);
    int insertMessagesGracefully(const QList<Message> &); // inserts as many contiguous msgs as possible. returns numer of inserted msgs.
    int indexForId(MsgId);

    //  QList<MessageModelItem *> _messageList;
    QList<Message> _messageBuffer;
    QTimer _dayChangeTimer;
    QDateTime _nextDayChange;
    QHash<BufferId, int> _messagesWaiting;
};


// inlines
QModelIndex MessageModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row < 0 || row >= rowCount(parent) || column < 0 || column >= columnCount(parent))
        return QModelIndex();

    return createIndex(row, column);
}


// **************************************************
//  MessageModelItem
// **************************************************
class MessageModelItem
{
public:
    //! Creates a MessageModelItem from a Message object.
    /** This baseclass implementation takes care of all Message data *except* the stylable strings.
     *  Subclasses need to provide Qt::DisplayRole at least, which should describe the plaintext
     *  strings without formattings (e.g. for searching purposes).
     */
    MessageModelItem() {}
    inline virtual ~MessageModelItem() {}

    virtual QVariant data(int column, int role) const;
    virtual bool setData(int column, const QVariant &value, int role);

    virtual const Message &message() const = 0;
    virtual const QDateTime &timestamp() const = 0;
    virtual const MsgId &msgId() const = 0;
    virtual const BufferId &bufferId() const = 0;
    virtual void setBufferId(BufferId bufferId) = 0;
    virtual Message::Type msgType() const = 0;
    virtual Message::Flags msgFlags() const = 0;

    // For sorting
    bool operator<(const MessageModelItem &) const;
    bool operator==(const MessageModelItem &) const;
    bool operator>(const MessageModelItem &) const;
    static bool lessThan(const MessageModelItem *m1, const MessageModelItem *m2);

private:
    BufferId _redirectedTo;
};


QDebug operator<<(QDebug dbg, const MessageModelItem &msgItem);

#endif
