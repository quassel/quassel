/***************************************************************************
 *   Copyright (C) 2005-2025 by the Quassel Project                        *
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

#ifndef IRCCHANNEL_H
#define IRCCHANNEL_H

#include <optional>

#include <QObject>
#include <QStringList>

#include "syncableobject.h"
#include "types.h"

class IrcUser;
class Network;

class COMMON_EXPORT IrcChannel : public SyncableObject
{
    Q_OBJECT
    SYNCABLE_OBJECT

public:
    IrcChannel(const QString& channelname, Network* network);

    QString name() const { return _name; }
    QString topic() const { return _topic; }
    QString password() const { return _password; }
    bool encrypted() const { return _encrypted; }
    QString modes() const;

    inline std::optional<QStringConverter::Encoding> codecForEncoding() const { return _codecForEncoding; }
    inline std::optional<QStringConverter::Encoding> codecForDecoding() const { return _codecForDecoding; }
    void setCodecForEncoding(const QString& name);
    void setCodecForDecoding(const QString& name);

    QString decodeString(const QByteArray& text) const;
    QByteArray encodeString(const QString& string) const;

    Network* network() const { return _network; }

    bool hasUser(IrcUser* user) const;
    QString userModes(IrcUser* user) const;
    QStringList userList() const;
    QList<IrcUser*> ircUsers() const;  // Added for tabcompleter.cpp

    QVariantMap toVariantMap() override;
    void fromVariantMap(const QVariantMap& map) override;

public slots:
    void setTopic(const QString& topic);
    void setPassword(const QString& password);
    void setEncrypted(bool encrypted);
    void joinIrcUsers(const QStringList& nicks, const QStringList& modes);
    void joinIrcUser(IrcUser* user);
    void part(IrcUser* user);
    void part(const QString& nick);
    void partChannel();
    void setUserModes(IrcUser* user, const QString& modes);
    void addUserMode(IrcUser* user, const QString& mode);
    Q_INVOKABLE void addUserMode(const QString& nick, const QString& mode);
    void removeUserMode(IrcUser* user, const QString& mode);
    void addChannelMode(const QChar& mode, const QString& value);
    void removeChannelMode(const QChar& mode, const QString& value);

signals:
    void topicChanged(const QString& topic);
    void encryptedChanged(bool encrypted);
    void usersJoined(const QStringList& nicks, const QStringList& modes);
    void userParted(IrcUser* user);
    void parted();
    void userModesChanged(IrcUser* user, const QString& modes);
    void userModeAdded(IrcUser* user, const QString& mode);
    void userModeRemoved(IrcUser* user, const QString& mode);

private slots:
    void ircUserNickSet(const QString& newnick);
    void userDestroyed();

private:
    QString _name;
    QString _topic;
    QString _password;
    bool _encrypted;
    Network* _network;
    std::optional<QStringConverter::Encoding> _codecForEncoding;
    std::optional<QStringConverter::Encoding> _codecForDecoding;
    QHash<IrcUser*, QString> _userModes;
    QHash<QChar, QStringList> _A_channelModes;
    QHash<QChar, QString> _B_channelModes;
    QHash<QChar, QString> _C_channelModes;
    QSet<QChar> _D_channelModes;

    friend class Network;
};

#endif  // IRCCHANNEL_H
