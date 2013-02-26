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

#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#include <QByteArray>
#include <QDateTime>
#include <QVariantList>

namespace Protocol {

enum Handler {
    SignalProxy
};


/*** handled by SignalProxy ***/

struct SignalProxyMessage
{
    inline Handler handler() const { return SignalProxy; }
};


struct SyncMessage : public SignalProxyMessage
{
    inline SyncMessage(const QByteArray &className, const QString &objectName, const QByteArray &slotName, const QVariantList &params)
    : className(className), objectName(objectName), slotName(slotName), params(params) {}

    QByteArray className;
    QString objectName;
    QByteArray slotName;
    QVariantList params;
};


struct RpcCall : public SignalProxyMessage
{
    inline RpcCall(const QByteArray &slotName, const QVariantList &params)
    : slotName(slotName), params(params) {}

    QByteArray slotName;
    QVariantList params;
};


struct InitRequest : public SignalProxyMessage
{
    inline InitRequest(const QByteArray &className, const QString &objectName)
    : className(className), objectName(objectName) {}

    QByteArray className;
    QString objectName;
};


struct InitData : public SignalProxyMessage
{
    inline InitData(const QByteArray &className, const QString &objectName, const QVariantMap &initData)
    : className(className), objectName(objectName), initData(initData) {}

    QByteArray className;
    QString objectName;
    QVariantMap initData;
};


/*** handled by RemoteConnection ***/

struct HeartBeat
{
    inline HeartBeat(const QDateTime &timestamp) : timestamp(timestamp) {}

    QDateTime timestamp;
};


struct HeartBeatReply
{
    inline HeartBeatReply(const QDateTime &timestamp) : timestamp(timestamp) {}

    QDateTime timestamp;
};


};

#endif
