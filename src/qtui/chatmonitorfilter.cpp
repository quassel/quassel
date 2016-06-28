/***************************************************************************
 *   Copyright (C) 2005-2016 by the Quassel Project                        *
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

#include "chatmonitorfilter.h"

#include "client.h"
#include "chatlinemodel.h"
#include "networkmodel.h"
#include "chatviewsettings.h"
#include "clientignorelistmanager.h"

ChatMonitorFilter::ChatMonitorFilter(MessageModel *model, QObject *parent)
    : MessageFilter(model, parent)
{
    ChatViewSettings viewSettings(idString());
    _showFields = viewSettings.value("ShowFields", AllFields).toInt();
    _showOwnMessages = viewSettings.value("ShowOwnMsgs", true).toBool();
    viewSettings.notify("ShowFields", this, SLOT(showFieldsSettingChanged(const QVariant &)));
    viewSettings.notify("ShowOwnMsgs", this, SLOT(showOwnMessagesSettingChanged(const QVariant &)));

    // ChatMonitorSettingsPage
    QString showHighlightsSettingsId = "ShowHighlights";
    QString operationModeSettingsId = "OperationMode";
    QString buffersSettingsId = "Buffers";
    QString showBacklogSettingsId = "ShowBacklog";
    QString includeReadSettingsId = "IncludeRead";

    _showHighlights = viewSettings.value(showHighlightsSettingsId, false).toBool();
    _operationMode = viewSettings.value(operationModeSettingsId, 0).toInt();
    // read configured list of buffers to monitor/ignore
    foreach(QVariant v, viewSettings.value(buffersSettingsId, QVariant()).toList())
    _bufferIds << v.value<BufferId>();
    _showBacklog = viewSettings.value(showBacklogSettingsId, true).toBool();
    _includeRead = viewSettings.value(includeReadSettingsId, true).toBool();

    viewSettings.notify(showHighlightsSettingsId, this, SLOT(showHighlightsSettingChanged(const QVariant &)));
    viewSettings.notify(operationModeSettingsId, this, SLOT(operationModeSettingChanged(const QVariant &)));
    viewSettings.notify(buffersSettingsId, this, SLOT(buffersSettingChanged(const QVariant &)));
    viewSettings.notify(showBacklogSettingsId, this, SLOT(showBacklogSettingChanged(const QVariant &)));
    viewSettings.notify(includeReadSettingsId, this, SLOT(includeReadSettingChanged(const QVariant &)));
}


bool ChatMonitorFilter::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    Q_UNUSED(sourceParent)

    QModelIndex source_index = sourceModel()->index(sourceRow, 0);
    BufferId bufferId = source_index.data(MessageModel::BufferIdRole).value<BufferId>();

    Message::Flags flags = (Message::Flags)source_index.data(MessageModel::FlagsRole).toInt();
    if ((flags & Message::Backlog) && (!_showBacklog || (!_includeRead &&
        (Client::networkModel()->lastSeenMsgId(bufferId) >= sourceModel()->data(source_index, MessageModel::MsgIdRole).value<MsgId>()))))
        return false;

    if (!_showOwnMessages && flags & Message::Self)
        return false;

    Message::Type type = (Message::Type)source_index.data(MessageModel::TypeRole).toInt();
    if (!(type & (Message::Plain | Message::Notice | Message::Action)))
        return false;

    // ChatMonitorSettingsPage
    if (_operationMode == ChatViewSettings::OptOut
        && !(_showHighlights && flags & Message::Highlight)
        &&  _bufferIds.contains(bufferId))
        return false;
    if (_operationMode == ChatViewSettings::OptIn
        && !(_showHighlights && flags & Message::Highlight)
        && !_bufferIds.contains(bufferId))
        return false;

    // ignorelist handling
    // only match if message is not flagged as server msg
    if (!(flags & Message::ServerMsg) && Client::ignoreListManager()
        && Client::ignoreListManager()->match(source_index.data(MessageModel::MessageRole).value<Message>(), Client::networkModel()->networkName(bufferId)))
        return false;
    return true;
}


// override this to inject display of network and channel
QVariant ChatMonitorFilter::data(const QModelIndex &index, int role) const
{
    if (index.column() != ChatLineModel::SenderColumn || role != ChatLineModel::DisplayRole)
        return MessageFilter::data(index, role);

    BufferId bufid = data(index, ChatLineModel::BufferIdRole).value<BufferId>();
    if (!bufid.isValid()) {
        qDebug() << "ChatMonitorFilter::data(): chatline belongs to an invalid buffer!";
        return QVariant();
    }

    QModelIndex source_index = mapToSource(index);

    QStringList fields;
    if (_showFields & NetworkField) {
        fields << Client::networkModel()->networkName(bufid);
    }
    if (_showFields & BufferField) {
        fields << Client::networkModel()->bufferName(bufid);
    }

    Message::Type messageType = (Message::Type)source_index.data(MessageModel::TypeRole).toInt();
    if (messageType & (Message::Plain | Message::Notice)) {
        QString sender = MessageFilter::data(index, ChatLineModel::EditRole).toString();
        fields << sender;
    }
    return QString("%1").arg(fields.join(" "));
}


void ChatMonitorFilter::addShowField(int field)
{
    if (_showFields & field)
        return;

    ChatViewSettings(idString()).setValue("ShowFields", _showFields | field);
}


void ChatMonitorFilter::removeShowField(int field)
{
    if (!(_showFields & field))
        return;

    ChatViewSettings(idString()).setValue("ShowFields", _showFields ^ field);
}


void ChatMonitorFilter::setShowOwnMessages(bool show)
{
    if (_showOwnMessages == show)
        return;

    ChatViewSettings(idString()).setValue("ShowOwnMsgs", show);
}


void ChatMonitorFilter::showFieldsSettingChanged(const QVariant &newValue)
{
    int newFields = newValue.toInt();
    if (_showFields == newFields)
        return;

    _showFields = newFields;

    int rows = rowCount();
    if (rows == 0)
        return;

    emit dataChanged(index(0, ChatLineModel::SenderColumn), index(rows - 1, ChatLineModel::SenderColumn));
}


void ChatMonitorFilter::showOwnMessagesSettingChanged(const QVariant &newValue)
{
    _showOwnMessages = newValue.toBool();
}


void ChatMonitorFilter::showHighlightsSettingChanged(const QVariant &newValue)
{
    _showHighlights = newValue.toBool();
}


void ChatMonitorFilter::operationModeSettingChanged(const QVariant &newValue)
{
    _operationMode = newValue.toInt();
}


void ChatMonitorFilter::buffersSettingChanged(const QVariant &newValue)
{
    _bufferIds.clear();
    foreach(QVariant v, newValue.toList()) {
        _bufferIds << v.value<BufferId>();
    }
    invalidateFilter();
}

void ChatMonitorFilter::showBacklogSettingChanged(const QVariant &newValue) {
    _showBacklog = newValue.toBool();
}

void ChatMonitorFilter::includeReadSettingChanged(const QVariant &newValue) {
    _includeRead = newValue.toBool();
}
