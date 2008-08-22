/***************************************************************************
*   Copyright (C) 2005-08 by the Quassel Project                          *
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
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

#ifndef CHATMONITORFILTER_H_
#define CHATMONITORFILTER_H_

#include <QDateTime>

#include "messagefilter.h"
#include "qtuisettings.h"

class ChatMonitorFilter : public MessageFilter {
  Q_OBJECT

public:
  enum SenderFields {
    NoField = 0x00,
    NetworkField = 0x01,
    BufferField = 0x02,
    SenderField = 0x04,
    AllFields = 0xFF
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
  void showFieldsSettingsChanged(const QVariant &newValue);
  void showOwnMessagesSettingChanged(const QVariant &newValue);

private:
  int _showFields;
  bool _showOwnMessages;
};

#endif
