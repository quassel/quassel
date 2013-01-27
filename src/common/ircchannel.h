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

#ifndef IRCCHANNEL_H
#define IRCCHANNEL_H

#include <QHash>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVariantMap>

#include "syncableobject.h"

class IrcUser;
class Network;

class IrcChannel : public SyncableObject
{
    SYNCABLE_OBJECT
    Q_OBJECT

    Q_PROPERTY(QString name READ name STORED false)
    Q_PROPERTY(QString topic READ topic WRITE setTopic STORED false)
    Q_PROPERTY(QString password READ password WRITE setPassword STORED false)

public :
        IrcChannel(const QString &channelname, Network *network);
    ~IrcChannel();

    bool isKnownUser(IrcUser *ircuser) const;
    bool isValidChannelUserMode(const QString &mode) const;

    inline QString name() const { return _name; }
    inline QString topic() const { return _topic; }
    inline QString password() const { return _password; }
    inline Network *network() const { return _network; }

    inline QList<IrcUser *> ircUsers() const { return _userModes.keys(); }

    QString userModes(IrcUser *ircuser) const;
    QString userModes(const QString &nick) const;

    bool hasMode(const QChar &mode) const;
    QString modeValue(const QChar &mode) const;
    QStringList modeValueList(const QChar &mode) const;
    QString channelModeString() const;

    inline QTextCodec *codecForEncoding() const { return _codecForEncoding; }
    inline QTextCodec *codecForDecoding() const { return _codecForDecoding; }
    void setCodecForEncoding(const QString &codecName);
    void setCodecForEncoding(QTextCodec *codec);
    void setCodecForDecoding(const QString &codecName);
    void setCodecForDecoding(QTextCodec *codec);

    QString decodeString(const QByteArray &text) const;
    QByteArray encodeString(const QString &string) const;

public slots:
    void setTopic(const QString &topic);
    void setPassword(const QString &password);

    void joinIrcUsers(const QList<IrcUser *> &users, const QStringList &modes);
    void joinIrcUsers(const QStringList &nicks, const QStringList &modes);
    void joinIrcUser(IrcUser *ircuser);

    void part(IrcUser *ircuser);
    void part(const QString &nick);

    void setUserModes(IrcUser *ircuser, const QString &modes);
    void setUserModes(const QString &nick, const QString &modes);

    void addUserMode(IrcUser *ircuser, const QString &mode);
    void addUserMode(const QString &nick, const QString &mode);

    void removeUserMode(IrcUser *ircuser, const QString &mode);
    void removeUserMode(const QString &nick, const QString &mode);

    void addChannelMode(const QChar &mode, const QString &value);
    void removeChannelMode(const QChar &mode, const QString &value);

    // init geters
    QVariantMap initUserModes() const;
    QVariantMap initChanModes() const;

    // init seters
    void initSetUserModes(const QVariantMap &usermodes);
    void initSetChanModes(const QVariantMap &chanModes);

signals:
    void topicSet(const QString &topic); // needed by NetworkModel
//   void passwordSet(const QString &password);
//   void userModesSet(QString nick, QString modes);
//   void userModeAdded(QString nick, QString mode);
//   void userModeRemoved(QString nick, QString mode);
//   void channelModeAdded(const QChar &mode, const QString &value);
//   void channelModeRemoved(const QChar &mode, const QString &value);

    void ircUsersJoined(QList<IrcUser *> ircusers);
//   void ircUsersJoined(QStringList nicks, QStringList modes);
    void ircUserParted(IrcUser *ircuser);
    void ircUserNickSet(IrcUser *ircuser, QString nick);
    void ircUserModeAdded(IrcUser *ircuser, QString mode);
    void ircUserModeRemoved(IrcUser *ircuser, QString mode);
    void ircUserModesSet(IrcUser *ircuser, QString modes);

    void parted(); // convenience signal emitted before channels destruction

private slots:
    void ircUserDestroyed();
    void ircUserNickSet(QString nick);

private:
    bool _initialized;
    QString _name;
    QString _topic;
    QString _password;

    QHash<IrcUser *, QString> _userModes;

    Network *_network;

    QTextCodec *_codecForEncoding;
    QTextCodec *_codecForDecoding;

    QHash<QChar, QStringList> _A_channelModes;
    QHash<QChar, QString> _B_channelModes;
    QHash<QChar, QString> _C_channelModes;
    QSet<QChar> _D_channelModes;
};


#endif
