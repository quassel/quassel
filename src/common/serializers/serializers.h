/***************************************************************************
 *   Copyright (C) 2005-2022 by the Quassel Project                        *
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

#pragma once

#include <QDataStream>
#include <QHostAddress>
#include <QTime>
#include <QVariantList>
#include <QtEndian>

#include "bufferinfo.h"
#include "identity.h"
#include "message.h"
#include "network.h"
#include "peer.h"

namespace Serializers {
namespace Types {
enum class VariantType : quint32
{
    Void = 0,
    Bool = 1,
    Int = 2,
    UInt = 3,

    QChar = 7,
    QVariantMap = 8,
    QVariantList = 9,
    QString = 10,
    QStringList = 11,
    QByteArray = 12,

    QDate = 14,
    QTime = 15,
    QDateTime = 16,

    Long = 129,
    Short = 130,
    Char = 131,
    ULong = 132,
    UShort = 133,
    UChar = 134,

    QVariant = 138,

    UserType = 127
};

enum class QuasselType
{
    Invalid,
    BufferId,
    BufferInfo,
    Identity,
    IdentityId,
    Message,
    MsgId,
    NetworkId,
    NetworkInfo,
    Network_Server,
    PeerPtr
};

VariantType variantType(QuasselType type);
QString toName(QuasselType type);
Types::QuasselType fromName(::QByteArray& name);
}  // namespace Types

bool deserialize(QDataStream& stream, const Quassel::Features& features, QVariant& data);
bool deserialize(QDataStream& stream, const Quassel::Features& features, QVariantList& list);
bool deserialize(QDataStream& stream, const Quassel::Features& features, QVariantMap& data);
bool deserialize(QDataStream& stream, const Quassel::Features& features, QVariant& data, Types::VariantType type);
bool deserialize(QDataStream& stream, const Quassel::Features& features, QVariant& data, Types::QuasselType type);
bool deserialize(QDataStream& stream, const Quassel::Features& features, bool& data);
bool deserialize(QDataStream& stream, const Quassel::Features& features, int8_t& data);
bool deserialize(QDataStream& stream, const Quassel::Features& features, uint8_t& data);
bool deserialize(QDataStream& stream, const Quassel::Features& features, int16_t& data);
bool deserialize(QDataStream& stream, const Quassel::Features& features, uint16_t& data);
bool deserialize(QDataStream& stream, const Quassel::Features& features, int32_t& data);
bool deserialize(QDataStream& stream, const Quassel::Features& features, uint32_t& data);
bool deserialize(QDataStream& stream, const Quassel::Features& features, qlonglong& data);
bool deserialize(QDataStream& stream, const Quassel::Features& features, qulonglong& data);
bool deserialize(QDataStream& stream, const Quassel::Features& features, Types::VariantType& data);
bool deserialize(QDataStream& stream, const Quassel::Features& features, QChar& data);
bool deserialize(QDataStream& stream, const Quassel::Features& features, QString& data);
bool deserialize(QDataStream& stream, const Quassel::Features& features, QTime& data);
bool deserialize(QDataStream& stream, const Quassel::Features& features, QDate& data);
bool deserialize(QDataStream& stream, const Quassel::Features& features, QDateTime& data);
bool deserialize(QDataStream& stream, const Quassel::Features& features, QByteArray& data);
bool deserialize(QDataStream& stream, const Quassel::Features& features, QStringList& data);
bool deserialize(QDataStream& stream, const Quassel::Features& features, Message& data);
bool deserialize(QDataStream& stream, const Quassel::Features& features, BufferInfo& data);
bool deserialize(QDataStream& stream, const Quassel::Features& features, BufferId& data);
bool deserialize(QDataStream& stream, const Quassel::Features& features, IdentityId& data);
bool deserialize(QDataStream& stream, const Quassel::Features& features, NetworkId& data);
bool deserialize(QDataStream& stream, const Quassel::Features& features, MsgId& data);
bool deserialize(QDataStream& stream, const Quassel::Features& features, PeerPtr& data);
bool deserialize(QDataStream& stream, const Quassel::Features& features, NetworkInfo& data);
bool deserialize(QDataStream& stream, const Quassel::Features& features, Identity& data);
bool deserialize(QDataStream& stream, const Quassel::Features& features, Network::Server& data);
}  // namespace Serializers
