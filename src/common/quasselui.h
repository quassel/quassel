/***************************************************************************
 *   Copyright (C) 2005-07 by The Quassel IRC Development Team             *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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

#ifndef _QUASSELUI_H_
#define _QUASSELUI_H_

#include <QObject>
#include "message.h"

class AbstractUiMsg {

  public:
    //virtual ~AbstractUiMsg() {};
    virtual QString sender() const = 0;
    virtual QString text() const = 0;
    virtual MsgId msgId() const = 0;
    virtual BufferId bufferId() const = 0;
    virtual QDateTime timeStamp() const = 0;

};


class AbstractUi : public QObject {
  Q_OBJECT

  public:
    virtual void init() {};  // called after the client is initialized
    virtual AbstractUiMsg *layoutMsg(const Message &) = 0;

  protected slots:
    virtual void connectedToCore() {}
    virtual void disconnectedFromCore() {}

  signals:
    void connectToCore(const QVariantMap &connInfo);
    void disconnectFromCore();

};



#endif
