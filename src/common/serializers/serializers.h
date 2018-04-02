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
        enum class VariantType : quint32 {
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

        enum class QuasselType {
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
        Types::QuasselType fromName(::QByteArray &name);
    }

    bool deserialize(QDataStream &stream, QVariant &data);
    bool deserialize(QDataStream &stream, QVariantList &list);
    bool deserialize(QDataStream &stream, QVariantMap &data);
    bool deserialize(QDataStream &stream, QVariant &data, Types::VariantType type);
    bool deserialize(QDataStream &stream, QVariant &data, Types::QuasselType type);
    bool deserialize(QDataStream &stream, bool &data);
    bool deserialize(QDataStream &stream, int8_t &data);
    bool deserialize(QDataStream &stream, uint8_t &data);
    bool deserialize(QDataStream &stream, int16_t &data);
    bool deserialize(QDataStream &stream, uint16_t &data);
    bool deserialize(QDataStream &stream, int32_t &data);
    bool deserialize(QDataStream &stream, uint32_t &data);
    bool deserialize(QDataStream &stream, qlonglong &data);
    bool deserialize(QDataStream &stream, qulonglong &data);
    bool deserialize(QDataStream &stream, Types::VariantType &data);
    bool deserialize(QDataStream &stream, QChar &data);
    bool deserialize(QDataStream &stream, QString &data);
    bool deserialize(QDataStream &stream, QTime &data);
    bool deserialize(QDataStream &stream, QDate &data);
    bool deserialize(QDataStream &stream, QDateTime &data);
    bool deserialize(QDataStream &stream, QByteArray &data);
    bool deserialize(QDataStream &stream, QStringList &data);
    bool deserialize(QDataStream &stream, Message &data);
    bool deserialize(QDataStream &stream, BufferInfo &data);
    bool deserialize(QDataStream &stream, BufferId &data);
    bool deserialize(QDataStream &stream, IdentityId &data);
    bool deserialize(QDataStream &stream, NetworkId &data);
    bool deserialize(QDataStream &stream, MsgId &data);
    bool deserialize(QDataStream &stream, PeerPtr &data);
    bool deserialize(QDataStream &stream, NetworkInfo &data);
    bool deserialize(QDataStream &stream, Identity &data);
    bool deserialize(QDataStream &stream, Network::Server &data);
};
