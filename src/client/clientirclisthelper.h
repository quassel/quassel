/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
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

#ifndef CLIENTIRCLISTHELPER_H
#define CLIENTIRCLISTHELPER_H

#include "irclisthelper.h"

class ClientIrcListHelper : public IrcListHelper {
  Q_OBJECT

public:
  inline ClientIrcListHelper(QObject *object = 0) : IrcListHelper(object) {};

  inline virtual const QMetaObject *syncMetaObject() const { return &IrcListHelper::staticMetaObject; }

public slots:
  virtual QVariantList requestChannelList(const NetworkId &netId, const QStringList &channelFilters);
  virtual void receiveChannelList(const NetworkId &netId, const QStringList &channelFilters, const QVariantList &channels);
  virtual void reportFinishedList(const NetworkId &netId);

signals:
  void channelListReceived(const NetworkId &netId, const QStringList &channelFilters, const QList<IrcListHelper::ChannelDescription> &channelList);

private:
  NetworkId _netId;
};

#endif //CLIENTIRCLISTHELPER_H
