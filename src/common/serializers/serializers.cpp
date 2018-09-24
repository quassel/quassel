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

#include "serializers.h"

bool checkStreamValid(QDataStream& stream)
{
    if (stream.status() != QDataStream::Ok) {
        qWarning() << "Peer sent corrupt data";
        return false;
    }

    return true;
}

bool Serializers::deserialize(QDataStream& stream, const Quassel::Features& features, QVariantList& data)
{
    uint32_t size;
    if (!deserialize(stream, features, size))
        return false;
    if (size > 4 * 1024 * 1024) {
        qWarning() << "Peer sent too large QVariantList: " << size;
        return false;
    }
    for (uint32_t i = 0; i < size; i++) {
        QVariant element;
        if (!deserialize(stream, features, element))
            return false;
        data << element;
    }
    return checkStreamValid(stream);
}

bool Serializers::deserialize(QDataStream& stream, const Quassel::Features& features, QVariantMap& data)
{
    uint32_t size;
    if (!deserialize(stream, features, size))
        return false;
    if (size > 4 * 1024 * 1024) {
        qWarning() << "Peer sent too large QVariantMap: " << size;
        return false;
    }
    for (uint32_t i = 0; i < size; i++) {
        QString key;
        QVariant value;
        if (!deserialize(stream, features, key))
            return false;
        if (!deserialize(stream, features, value))
            return false;
        data[key] = value;
    }
    return checkStreamValid(stream);
}

bool Serializers::deserialize(QDataStream& stream, const Quassel::Features& features, QVariant& data)
{
    Types::VariantType type;
    int8_t isNull;
    if (!deserialize(stream, features, type))
        return false;
    if (!deserialize(stream, features, isNull))
        return false;
    if (type == Types::VariantType::UserType) {
        QByteArray name;
        if (!deserialize(stream, features, name))
            return false;
        while (name.length() > 0 && name.at(name.length() - 1) == 0)
            name.chop(1);
        if (!deserialize(stream, features, data, Types::fromName(name)))
            return false;
    }
    else {
        if (!deserialize(stream, features, data, type))
            return false;
    }
    return checkStreamValid(stream);
}

bool Serializers::deserialize(QDataStream& stream, const Quassel::Features& features, QVariant& data, Types::VariantType type)
{
    switch (type) {
    case Types::VariantType::Void: {
        return true;
    }
    case Types::VariantType::Bool: {
        bool content;
        if (!deserialize(stream, features, content))
            return false;
        data = QVariant(content);
        return true;
    }
    case Types::VariantType::Int: {
        int32_t content;
        if (!deserialize(stream, features, content))
            return false;
        data = QVariant(content);
        return true;
    }
    case Types::VariantType::UInt: {
        uint32_t content;
        if (!deserialize(stream, features, content))
            return false;
        data = QVariant(content);
        return true;
    }
    case Types::VariantType::QChar: {
        QChar content;
        if (!deserialize(stream, features, content))
            return false;
        data = QVariant(content);
        return true;
    }
    case Types::VariantType::QVariantMap: {
        QVariantMap content;
        if (!deserialize(stream, features, content))
            return false;
        data = QVariant(content);
        return true;
    }
    case Types::VariantType::QVariantList: {
        QVariantList content;
        if (!deserialize(stream, features, content))
            return false;
        data = QVariant(content);
        return true;
    }
    case Types::VariantType::QString: {
        QString content;
        if (!deserialize(stream, features, content))
            return false;
        data = QVariant(content);
        return true;
    }
    case Types::VariantType::QStringList: {
        QStringList content;
        if (!deserialize(stream, features, content))
            return false;
        data = QVariant(content);
        return true;
    }
    case Types::VariantType::QByteArray: {
        QByteArray content;
        if (!deserialize(stream, features, content))
            return false;
        data = QVariant(content);
        return true;
    }
    case Types::VariantType::QDate: {
        QDate content;
        if (!deserialize(stream, features, content))
            return false;
        data = QVariant(content);
        return true;
    }
    case Types::VariantType::QTime: {
        QTime content;
        if (!deserialize(stream, features, content))
            return false;
        data = QVariant(content);
        return true;
    }
    case Types::VariantType::QDateTime: {
        QDateTime content;
        if (!deserialize(stream, features, content))
            return false;
        data = QVariant(content);
        return true;
    }
    case Types::VariantType::Long: {
        qlonglong content;
        if (!deserialize(stream, features, content))
            return false;
        data = QVariant(content);
        return true;
    }
    case Types::VariantType::Short: {
        int16_t content;
        if (!deserialize(stream, features, content))
            return false;
        data = QVariant(content);
        return true;
    }
    case Types::VariantType::Char: {
        int8_t content;
        if (!deserialize(stream, features, content))
            return false;
        data = QVariant(content);
        return true;
    }
    case Types::VariantType::ULong: {
        qulonglong content;
        if (!deserialize(stream, features, content))
            return false;
        data = QVariant(content);
        return true;
    }
    case Types::VariantType::UShort: {
        uint16_t content;
        if (!deserialize(stream, features, content))
            return false;
        data = QVariant(content);
        return true;
    }
    case Types::VariantType::UChar: {
        uint8_t content;
        if (!deserialize(stream, features, content))
            return false;
        data = QVariant(content);
        return true;
    }
    case Types::VariantType::QVariant: {
        QVariant content;
        if (!deserialize(stream, features, content))
            return false;
        data = QVariant(content);
        return true;
    }
    default: {
        qWarning() << "Usertype should have been caught earlier already";
        return false;
    }
    }
}

bool Serializers::deserialize(QDataStream& stream, const Quassel::Features& features, QVariant& data, Types::QuasselType type)
{
    switch (type) {
    case Types::QuasselType::BufferId: {
        BufferId content;
        if (!deserialize(stream, features, content))
            return false;
        data = qVariantFromValue(content);
        return true;
    }
    case Types::QuasselType::BufferInfo: {
        BufferInfo content;
        if (!deserialize(stream, features, content))
            return false;
        data = qVariantFromValue(content);
        return true;
    }
    case Types::QuasselType::Identity: {
        Identity content;
        if (!deserialize(stream, features, content))
            return false;
        data = qVariantFromValue(content);
        return true;
    }
    case Types::QuasselType::IdentityId: {
        IdentityId content;
        if (!deserialize(stream, features, content))
            return false;
        data = qVariantFromValue(content);
        return true;
    }
    case Types::QuasselType::Message: {
        Message content;
        if (!deserialize(stream, features, content))
            return false;
        data = qVariantFromValue(content);
        return true;
    }
    case Types::QuasselType::MsgId: {
        MsgId content;
        if (!deserialize(stream, features, content))
            return false;
        data = qVariantFromValue(content);
        return true;
    }
    case Types::QuasselType::NetworkId: {
        NetworkId content;
        if (!deserialize(stream, features, content))
            return false;
        data = qVariantFromValue(content);
        return true;
    }
    case Types::QuasselType::NetworkInfo: {
        NetworkInfo content;
        if (!deserialize(stream, features, content))
            return false;
        data = qVariantFromValue(content);
        return true;
    }
    case Types::QuasselType::Network_Server: {
        Network::Server content;
        if (!deserialize(stream, features, content))
            return false;
        data = qVariantFromValue(content);
        return true;
    }
    case Types::QuasselType::PeerPtr: {
        PeerPtr content;
        if (!deserialize(stream, features, content))
            return false;
        data = qVariantFromValue(content);
        return true;
    }
    default: {
        qWarning() << "Invalid QType";
        return false;
    }
    }
}

bool Serializers::deserialize(QDataStream& stream, const Quassel::Features& features, Types::VariantType& data)
{
    uint32_t raw;
    if (!deserialize(stream, features, raw))
        return false;
    data = static_cast<Types::VariantType>(raw);
    return checkStreamValid(stream);
}

bool Serializers::deserialize(QDataStream& stream, const Quassel::Features& features, QStringList& data)
{
    uint32_t size;
    if (!deserialize(stream, features, size))
        return false;
    for (uint32_t i = 0; i < size; i++) {
        QString element;
        if (!deserialize(stream, features, element))
            return false;
        data << element;
    }
    return checkStreamValid(stream);
}

bool Serializers::deserialize(QDataStream& stream, const Quassel::Features& features, Network::Server& server)
{
    Q_UNUSED(features);
    QVariantMap serverMap;
    if (!deserialize(stream, features, serverMap))
        return false;
    server.host = serverMap["Host"].toString();
    server.port = serverMap["Port"].toUInt();
    server.password = serverMap["Password"].toString();
    server.useSsl = serverMap["UseSSL"].toBool();
    server.sslVerify = serverMap["sslVerify"].toBool();
    server.sslVersion = serverMap["sslVersion"].toInt();
    server.useProxy = serverMap["UseProxy"].toBool();
    server.proxyType = serverMap["ProxyType"].toInt();
    server.proxyHost = serverMap["ProxyHost"].toString();
    server.proxyPort = serverMap["ProxyPort"].toUInt();
    server.proxyUser = serverMap["ProxyUser"].toString();
    server.proxyPass = serverMap["ProxyPass"].toString();
    return checkStreamValid(stream);
}

bool Serializers::deserialize(QDataStream& stream, const Quassel::Features& features, Identity& data)
{
    Q_UNUSED(features);
    QVariantMap raw;
    if (!deserialize(stream, features, raw))
        return false;
    data.fromVariantMap(raw);
    return checkStreamValid(stream);
}

bool Serializers::deserialize(QDataStream& stream, const Quassel::Features& features, NetworkInfo& info)
{
    Q_UNUSED(features);
    QVariantMap i;
    if (!deserialize(stream, features, i))
        return false;
    info.networkId = i["NetworkId"].value<NetworkId>();
    info.networkName = i["NetworkName"].toString();
    info.identity = i["Identity"].value<IdentityId>();
    info.codecForServer = i["CodecForServer"].toByteArray();
    info.codecForEncoding = i["CodecForEncoding"].toByteArray();
    info.codecForDecoding = i["CodecForDecoding"].toByteArray();
    info.serverList = fromVariantList<Network::Server>(i["ServerList"].toList());
    info.useRandomServer = i["UseRandomServer"].toBool();
    info.perform = i["Perform"].toStringList();
    info.useAutoIdentify = i["UseAutoIdentify"].toBool();
    info.autoIdentifyService = i["AutoIdentifyService"].toString();
    info.autoIdentifyPassword = i["AutoIdentifyPassword"].toString();
    info.useSasl = i["UseSasl"].toBool();
    info.saslAccount = i["SaslAccount"].toString();
    info.saslPassword = i["SaslPassword"].toString();
    info.useAutoReconnect = i["UseAutoReconnect"].toBool();
    info.autoReconnectInterval = i["AutoReconnectInterval"].toUInt();
    info.autoReconnectRetries = i["AutoReconnectRetries"].toInt();
    info.unlimitedReconnectRetries = i["UnlimitedReconnectRetries"].toBool();
    info.rejoinChannels = i["RejoinChannels"].toBool();
    // Custom rate limiting
    info.useCustomMessageRate = i["UseCustomMessageRate"].toBool();
    info.messageRateBurstSize = i["MessageRateBurstSize"].toUInt();
    info.messageRateDelay = i["MessageRateDelay"].toUInt();
    info.unlimitedMessageRate = i["UnlimitedMessageRate"].toBool();
    return checkStreamValid(stream);
}

bool Serializers::deserialize(QDataStream& stream, const Quassel::Features& features, PeerPtr& data)
{
    Q_UNUSED(features);
    stream >> data;
    return checkStreamValid(stream);
}

bool Serializers::deserialize(QDataStream& stream, const Quassel::Features& features, QByteArray& data)
{
    Q_UNUSED(features);
    data.clear();
    uint32_t length;
    if (!deserialize(stream, features, length))
        return false;
    // -1 - or 0xffffffff - is used for an empty byte array
    if (length == 0xffffffff) {
        return true;
    }

    // 64 MB should be enough
    if (length > 64 * 1024 * 1024) {
        qWarning() << "Peer sent too large QByteArray: " << length;
        return false;
    }

    const uint32_t Step = 1024 * 1024;
    uint32_t allocated = 0;
    do {
        int blockSize = std::min(Step, length - allocated);
        data.resize(allocated + blockSize);
        if (stream.readRawData(data.data() + allocated, blockSize) != blockSize) {
            data.clear();
            qWarning() << "BufferUnderFlow while reading QByteArray";
            return false;
        }
        allocated += blockSize;
    } while (allocated < length);
    return checkStreamValid(stream);
}

bool Serializers::deserialize(QDataStream& stream, const Quassel::Features& features, QString& data)
{
    Q_UNUSED(features);
    uint32_t bytes = 0;
    // read size of string
    if (!deserialize(stream, features, bytes))
        return false;

    // empty string
    if (bytes == 0) {
        return true;
    }

    // null string
    if (bytes == 0xffffffff) {
        data.clear();
        return true;
    }

    // 64 MB should be enough
    if (bytes > 64 * 1024 * 1024) {
        qWarning() << "Peer sent too large QString: " << bytes;
        return false;
    }

    if (bytes & 0x1) {
        data.clear();
        qWarning() << "Read corrupted data: UTF-6 String with odd length: " << bytes;
        return false;
    }
    const uint32_t step = 1024 * 1024;
    uint32_t length = bytes / 2;
    uint32_t allocated = 0;
    while (allocated < length) {
        int blockSize = std::min(step, length - allocated);
        data.resize(allocated + blockSize);
        if (stream.readRawData(reinterpret_cast<char*>(data.data()) + allocated * 2, blockSize * 2) != blockSize * 2) {
            data.clear();
            qWarning() << "BufferUnderFlow while reading QString";
            return false;
        }
        allocated += blockSize;
    }
    if ((stream.byteOrder() == QDataStream::BigEndian) != (QSysInfo::ByteOrder == QSysInfo::BigEndian)) {
        auto* rawData = reinterpret_cast<uint16_t*>(data.data());
        while (length--) {
            *rawData = qbswap(*rawData);
            ++rawData;
        }
    }
    return checkStreamValid(stream);
}

bool Serializers::deserialize(QDataStream& stream, const Quassel::Features& features, QChar& data)
{
    Q_UNUSED(features);
    stream >> data;
    return checkStreamValid(stream);
}

bool Serializers::deserialize(QDataStream& stream, const Quassel::Features& features, QDate& data)
{
    Q_UNUSED(features);
    stream >> data;
    return checkStreamValid(stream);
}

bool Serializers::deserialize(QDataStream& stream, const Quassel::Features& features, QTime& data)
{
    Q_UNUSED(features);
    stream >> data;
    return checkStreamValid(stream);
}

bool Serializers::deserialize(QDataStream& stream, const Quassel::Features& features, QDateTime& data)
{
    Q_UNUSED(features);
    stream >> data;
    return checkStreamValid(stream);
}

bool Serializers::deserialize(QDataStream& stream, const Quassel::Features& features, int32_t& data)
{
    Q_UNUSED(features);
    stream >> data;
    return checkStreamValid(stream);
}

bool Serializers::deserialize(QDataStream& stream, const Quassel::Features& features, uint32_t& data)
{
    Q_UNUSED(features);
    stream >> data;
    return checkStreamValid(stream);
}

bool Serializers::deserialize(QDataStream& stream, const Quassel::Features& features, int16_t& data)
{
    Q_UNUSED(features);
    stream >> data;
    return checkStreamValid(stream);
}

bool Serializers::deserialize(QDataStream& stream, const Quassel::Features& features, uint16_t& data)
{
    Q_UNUSED(features);
    stream >> data;
    return checkStreamValid(stream);
}

bool Serializers::deserialize(QDataStream& stream, const Quassel::Features& features, int8_t& data)
{
    Q_UNUSED(features);
    stream >> data;
    return checkStreamValid(stream);
}

bool Serializers::deserialize(QDataStream& stream, const Quassel::Features& features, uint8_t& data)
{
    Q_UNUSED(features);
    stream >> data;
    return checkStreamValid(stream);
}

bool Serializers::deserialize(QDataStream& stream, const Quassel::Features& features, qlonglong& data)
{
    Q_UNUSED(features);
    stream >> data;
    return checkStreamValid(stream);
}

bool Serializers::deserialize(QDataStream& stream, const Quassel::Features& features, qulonglong& data)
{
    Q_UNUSED(features);
    stream >> data;
    return checkStreamValid(stream);
}

bool Serializers::deserialize(QDataStream& stream, const Quassel::Features& features, bool& data)
{
    Q_UNUSED(features);
    stream >> data;
    return checkStreamValid(stream);
}

bool Serializers::deserialize(QDataStream& stream, const Quassel::Features& features, BufferInfo& data)
{
    Q_UNUSED(features);
    stream >> data;
    return checkStreamValid(stream);
}

bool Serializers::deserialize(QDataStream& stream, const Quassel::Features& features, Message& data)
{
    Q_UNUSED(features);
    stream >> data;
    return checkStreamValid(stream);
}

bool Serializers::deserialize(QDataStream& stream, const Quassel::Features& features, NetworkId& data)
{
    Q_UNUSED(features);
    stream >> data;
    return checkStreamValid(stream);
}

bool Serializers::deserialize(QDataStream& stream, const Quassel::Features& features, IdentityId& data)
{
    Q_UNUSED(features);
    stream >> data;
    return checkStreamValid(stream);
}

bool Serializers::deserialize(QDataStream& stream, const Quassel::Features& features, BufferId& data)
{
    Q_UNUSED(features);
    stream >> data;
    return checkStreamValid(stream);
}

bool Serializers::deserialize(QDataStream& stream, const Quassel::Features& features, MsgId& data)
{
    Q_UNUSED(features);
    stream >> data;
    return checkStreamValid(stream);
}

Serializers::Types::VariantType Serializers::Types::variantType(Serializers::Types::QuasselType type)
{
    switch (type) {
    case QuasselType::BufferId:
        return VariantType::UserType;
    case QuasselType::BufferInfo:
        return VariantType::UserType;
    case QuasselType::Identity:
        return VariantType::UserType;
    case QuasselType::IdentityId:
        return VariantType::UserType;
    case QuasselType::Message:
        return VariantType::UserType;
    case QuasselType::MsgId:
        return VariantType::UserType;
    case QuasselType::NetworkId:
        return VariantType::UserType;
    case QuasselType::NetworkInfo:
        return VariantType::UserType;
    case QuasselType::Network_Server:
        return VariantType::UserType;
    case QuasselType::PeerPtr:
        return VariantType::Long;
    default:
        return VariantType::UserType;
    }
}

QString Serializers::Types::toName(Serializers::Types::QuasselType type)
{
    switch (type) {
    case QuasselType::BufferId:
        return QString("BufferId");
    case QuasselType::BufferInfo:
        return QString("BufferInfo");
    case QuasselType::Identity:
        return QString("Identity");
    case QuasselType::IdentityId:
        return QString("IdentityId");
    case QuasselType::Message:
        return QString("Message");
    case QuasselType::MsgId:
        return QString("MsgId");
    case QuasselType::NetworkId:
        return QString("NetworkId");
    case QuasselType::NetworkInfo:
        return QString("NetworkInfo");
    case QuasselType::Network_Server:
        return QString("Network::Server");
    case QuasselType::PeerPtr:
        return QString("PeerPtr");
    default:
        return QString("Invalid Type");
    }
}

Serializers::Types::QuasselType Serializers::Types::fromName(::QByteArray& name)
{
    if (qstrcmp(name, "BufferId") == 0)
        return QuasselType::BufferId;
    else if (qstrcmp(name, "BufferInfo") == 0)
        return QuasselType::BufferInfo;
    else if (qstrcmp(name, "Identity") == 0)
        return QuasselType::Identity;
    else if (qstrcmp(name, "IdentityId") == 0)
        return QuasselType::IdentityId;
    else if (qstrcmp(name, "Message") == 0)
        return QuasselType::Message;
    else if (qstrcmp(name, "MsgId") == 0)
        return QuasselType::MsgId;
    else if (qstrcmp(name, "NetworkId") == 0)
        return QuasselType::NetworkId;
    else if (qstrcmp(name, "NetworkInfo") == 0)
        return QuasselType::NetworkInfo;
    else if (qstrcmp(name, "Network::Server") == 0)
        return QuasselType::Network_Server;
    else if (qstrcmp(name, "PeerPtr") == 0)
        return QuasselType::PeerPtr;
    else {
        qWarning() << "Type name is not valid: " << name;
        return QuasselType::Invalid;
    }
}
