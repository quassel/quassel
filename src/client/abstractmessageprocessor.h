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

#ifndef ABSTRACTMESSAGEPROCESSOR_H_
#define ABSTRACTMESSAGEPROCESSOR_H_

#include "client.h"
#include "message.h"
#include "networkmodel.h"

class AbstractMessageProcessor : public QObject
{
    Q_OBJECT

public:
    AbstractMessageProcessor(QObject *parent);
    virtual void reset() = 0;

public slots:
    virtual void process(Message &msg) = 0;
    virtual void process(QList<Message> &msgs) = 0;

protected:
    // updateBufferActivity also sets the Message::Redirected flag which is later used
    // to determine where a message should be displayed. therefore it's crucial that it
    // is called before inserting the message into the model
    inline void preProcess(Message &msg) { Client::networkModel()->updateBufferActivity(msg); }
};


#endif
