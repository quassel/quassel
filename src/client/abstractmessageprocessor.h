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

#ifndef ABSTRACTMESSAGEPROCESSOR_H_
#define ABSTRACTMESSAGEPROCESSOR_H_

#include "client.h"
#include "message.h"
#include "networkmodel.h"

class AbstractMessageProcessor : public QObject {
  Q_OBJECT

  public:
    AbstractMessageProcessor(QObject *parent);
    virtual void reset() = 0;

  public slots:
    virtual void process(Message &msg) = 0;
    virtual void process(QList<Message> &msgs) = 0;

  signals:
    void progressUpdated(int value, int maximum);

  protected:
    inline void postProcess(Message &msg) { Client::networkModel()->updateBufferActivity(msg); }

};

#endif
