/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
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

#include "messagefilter.h"

#include "buffersettings.h"
#include "client.h"
#include "buffermodel.h"
#include "messagemodel.h"
#include "networkmodel.h"
#include "clientignorelistmanager.h"

MessageFilter::MessageFilter(QAbstractItemModel *source, QObject *parent)
    : QSortFilterProxyModel(parent),
    _messageTypeFilter(0)
{
    init();
    setSourceModel(source);
}


MessageFilter::MessageFilter(MessageModel *source, const QList<BufferId> &buffers, QObject *parent)
    : QSortFilterProxyModel(parent),
    _validBuffers(buffers.toSet()),
    _messageTypeFilter(0)
{
    init();
    setSourceModel(source);
}


void MessageFilter::init()
{
    setDynamicSortFilter(true);

    _userNoticesTarget = _serverNoticesTarget = _errorMsgsTarget = -1;

    BufferSettings defaultSettings;
    defaultSettings.notify("UserNoticesTarget", this, SLOT(messageRedirectionChanged()));
    defaultSettings.notify("ServerNoticesTarget", this, SLOT(messageRedirectionChanged()));
    defaultSettings.notify("ErrorMsgsTarget", this, SLOT(messageRedirectionChanged()));
    messageRedirectionChanged();

    _messageTypeFilter = defaultSettings.messageFilter();
    defaultSettings.notify("MessageTypeFilter", this, SLOT(messageTypeFilterChanged()));

    BufferSettings mySettings(idString());
    if (mySettings.hasFilter())
        _messageTypeFilter = mySettings.messageFilter();
    mySettings.notify("MessageTypeFilter", this, SLOT(messageTypeFilterChanged()));
    mySettings.notify("hasMessageTypeFilter", this, SLOT(messageTypeFilterChanged()));
}


void MessageFilter::messageTypeFilterChanged()
{
    int newFilter;
    BufferSettings defaultSettings;
    newFilter = BufferSettings().messageFilter();

    BufferSettings mySettings(idString());
    if (mySettings.hasFilter())
        newFilter = mySettings.messageFilter();

    if (_messageTypeFilter != newFilter) {
        _messageTypeFilter = newFilter;
        _filteredQuitMsgs.clear();
        invalidateFilter();
    }
}


void MessageFilter::messageRedirectionChanged()
{
    BufferSettings bufferSettings;
    bool changed = false;

    if (_userNoticesTarget != bufferSettings.userNoticesTarget()) {
        _userNoticesTarget = bufferSettings.userNoticesTarget();
        changed = true;
    }

    if (_serverNoticesTarget != bufferSettings.serverNoticesTarget()) {
        _serverNoticesTarget = bufferSettings.serverNoticesTarget();
        changed = true;
    }

    if (_errorMsgsTarget != bufferSettings.errorMsgsTarget()) {
        _errorMsgsTarget = bufferSettings.errorMsgsTarget();
        changed = true;
    }

    if (changed)
        invalidateFilter();
}


QString MessageFilter::idString() const
{
    if (_validBuffers.isEmpty())
        return "*";

    QList<BufferId> bufferIds = _validBuffers.toList();
    qSort(bufferIds);

    QStringList bufferIdStrings;
    foreach(BufferId id, bufferIds)
    bufferIdStrings << QString::number(id.toInt());

    return bufferIdStrings.join("|");
}


bool MessageFilter::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    Q_UNUSED(sourceParent);
    QModelIndex sourceIdx = sourceModel()->index(sourceRow, 2);
    Message::Type messageType = (Message::Type)sourceIdx.data(MessageModel::TypeRole).toInt();

    // apply message type filter
    if (_messageTypeFilter & messageType)
        return false;

    if (_validBuffers.isEmpty())
        return true;

    BufferId bufferId = sourceIdx.data(MessageModel::BufferIdRole).value<BufferId>();
    if (!bufferId.isValid()) {
        return true;
    }

    // MsgId msgId = sourceIdx.data(MessageModel::MsgIdRole).value<MsgId>();
    Message::Flags flags = (Message::Flags)sourceIdx.data(MessageModel::FlagsRole).toInt();

    NetworkId myNetworkId = networkId();
    NetworkId msgNetworkId = Client::networkModel()->networkId(bufferId);
    if (myNetworkId != msgNetworkId)
        return false;

    // ignorelist handling
    // only match if message is not flagged as server msg
    if (!(flags & Message::ServerMsg) && Client::ignoreListManager()
        && Client::ignoreListManager()->match(sourceIdx.data(MessageModel::MessageRole).value<Message>(), Client::networkModel()->networkName(bufferId)))
        return false;

    if (flags & Message::Redirected) {
        int redirectionTarget = 0;
        switch (messageType) {
        case Message::Notice:
            if (Client::networkModel()->bufferType(bufferId) != BufferInfo::ChannelBuffer) {
                if (flags & Message::ServerMsg) {
                    // server notice
                    redirectionTarget = _serverNoticesTarget;
                }
                else {
                    redirectionTarget = _userNoticesTarget;
                }
            }
            break;
        case Message::Error:
            redirectionTarget = _errorMsgsTarget;
            break;
        default:
            break;
        }

        if (redirectionTarget & BufferSettings::DefaultBuffer && _validBuffers.contains(bufferId))
            return true;

        if (redirectionTarget & BufferSettings::CurrentBuffer && !(flags & Message::Backlog)) {
            BufferId redirectedTo = sourceModel()->data(sourceIdx, MessageModel::RedirectedToRole).value<BufferId>();
            if (!redirectedTo.isValid()) {
                BufferId redirectedTo = Client::bufferModel()->currentIndex().data(NetworkModel::BufferIdRole).value<BufferId>();
                if (redirectedTo.isValid())
                    sourceModel()->setData(sourceIdx, QVariant::fromValue<BufferId>(redirectedTo), MessageModel::RedirectedToRole);
            }

            if (_validBuffers.contains(redirectedTo))
                return true;
        }

        if (redirectionTarget & BufferSettings::StatusBuffer) {
            QSet<BufferId>::const_iterator idIter = _validBuffers.constBegin();
            while (idIter != _validBuffers.constEnd()) {
                if (Client::networkModel()->bufferType(*idIter) == BufferInfo::StatusBuffer)
                    return true;
                ++idIter;
            }
        }

        return false;
    }

    if (_validBuffers.contains(bufferId)) {
        return true;
    }
    else {
        // show Quit messages in Query buffers:
        if (bufferType() != BufferInfo::QueryBuffer)
            return false;
        if (!(messageType & Message::Quit))
            return false;

        if (myNetworkId != msgNetworkId)
            return false;

        qint64 messageTimestamp = sourceModel()->data(sourceIdx, MessageModel::TimestampRole)
                .value<QDateTime>().toMSecsSinceEpoch();
        QString quiter = sourceModel()->data(sourceIdx, Qt::DisplayRole).toString().section(' ', 0, 0, QString::SectionSkipEmpty).toLower();
        if (quiter != bufferName().toLower())
            return false;

        if (_filteredQuitMsgs.contains(quiter, messageTimestamp))
            return false;

        MessageFilter *that = const_cast<MessageFilter *>(this);
        that->_filteredQuitMsgs.insert(quiter,  messageTimestamp);
        return true;
    }
}


void MessageFilter::requestBacklog()
{
    QSet<BufferId>::const_iterator bufferIdIter = _validBuffers.constBegin();
    while (bufferIdIter != _validBuffers.constEnd()) {
        Client::messageModel()->requestBacklog(*bufferIdIter);
        ++bufferIdIter;
    }
}
