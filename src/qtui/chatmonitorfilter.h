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

#ifndef CHATMONITORFILTER_H_
#define CHATMONITORFILTER_H_

#include <QDateTime>

#include "messagefilter.h"

class ChatMonitorFilter : public MessageFilter
{
    Q_OBJECT

public:
    enum SenderFields {
        NoField = 0x00,
        NetworkField = 0x01,
        BufferField = 0x02,
        SenderField = 0x04,
        AllFields = 0xff
    };

    ChatMonitorFilter(MessageModel *model, QObject *parent = 0);

    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
    virtual QString idString() const { return "ChatMonitor"; }
    virtual QVariant data(const QModelIndex &index, int role) const;

    int showFields() const { return _showFields; }
    bool showOwnMessages() const { return _showOwnMessages; }

public slots:
    void addShowField(int field);
    void removeShowField(int field);
    void setShowOwnMessages(bool show);

private slots:
    void showFieldsSettingChanged(const QVariant &newValue);
    void showOwnMessagesSettingChanged(const QVariant &newValue);
    void showHighlightsSettingChanged(const QVariant &newValue);
    void operationModeSettingChanged(const QVariant &newValue);
    void buffersSettingChanged(const QVariant &newValue);
    void showBacklogSettingChanged(const QVariant &newValue);
    void includeReadSettingChanged(const QVariant &newValue);
    /**
     * Updates the local setting cache of whether or not to show sender brackets
     *
     * @param[in] newValue  If true, sender brackets are enabled, otherwise false.
     */
    void showSenderBracketsSettingChanged(const QVariant &newValue);

private:
    int _showFields;
    bool _showOwnMessages;
    QList<BufferId> _bufferIds;
    bool _showHighlights;
    int _operationMode;
    bool _showBacklog;
    bool _includeRead;
    bool _showSenderBrackets;   /// If true, show brackets around sender names
};


#endif
