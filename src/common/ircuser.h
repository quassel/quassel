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

#ifndef IRCUSER_H
#define IRCUSER_H

#include <QSet>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QDateTime>

#include "syncableobject.h"
#include "types.h"

class SignalProxy;
class Network;
class IrcChannel;

class IrcUser : public SyncableObject
{
    SYNCABLE_OBJECT
    Q_OBJECT

    Q_PROPERTY(QString user READ user WRITE setUser)
    Q_PROPERTY(QString host READ host WRITE setHost)
    Q_PROPERTY(QString nick READ nick WRITE setNick)
    Q_PROPERTY(QString realName READ realName WRITE setRealName)
    Q_PROPERTY(QString account READ account WRITE setAccount)
    Q_PROPERTY(bool away READ isAway WRITE setAway)
    Q_PROPERTY(QString awayMessage READ awayMessage WRITE setAwayMessage)
    Q_PROPERTY(QDateTime idleTime READ idleTime WRITE setIdleTime)
    Q_PROPERTY(QDateTime loginTime READ loginTime WRITE setLoginTime)
    Q_PROPERTY(QString server READ server WRITE setServer)
    Q_PROPERTY(QString ircOperator READ ircOperator WRITE setIrcOperator)
    Q_PROPERTY(QDateTime lastAwayMessage READ lastAwayMessage WRITE setLastAwayMessage)
    Q_PROPERTY(QString whoisServiceReply READ whoisServiceReply WRITE setWhoisServiceReply)
    Q_PROPERTY(QString suserHost READ suserHost WRITE setSuserHost)
    Q_PROPERTY(bool encrypted READ encrypted WRITE setEncrypted)

    Q_PROPERTY(QStringList channels READ channels)
    Q_PROPERTY(QString userModes READ userModes WRITE setUserModes)

public :
        IrcUser(const QString &hostmask, Network *network);
    virtual ~IrcUser();

    inline QString user() const { return _user; }
    inline QString host() const { return _host; }
    inline QString nick() const { return _nick; }
    inline QString realName() const { return _realName; }
    /**
     * Account name, e.g. NickServ/SASL account
     *
     * @return Account name if logged in, * if logged out, or empty string if unknown
     */
    inline QString account() const { return _account; }
    QString hostmask() const;
    inline bool isAway() const { return _away; }
    inline QString awayMessage() const { return _awayMessage; }
    QDateTime idleTime();
    inline QDateTime loginTime() const { return _loginTime; }
    inline QString server() const { return _server; }
    inline QString ircOperator() const { return _ircOperator; }
    inline QDateTime lastAwayMessage() const { return _lastAwayMessage; }
    inline QString whoisServiceReply() const { return _whoisServiceReply; }
    inline QString suserHost() const { return _suserHost; }
    inline bool encrypted() const { return _encrypted; }
    inline Network *network() const { return _network; }

    inline QString userModes() const { return _userModes; }

    QStringList channels() const;

    // user-specific encodings
    inline QTextCodec *codecForEncoding() const { return _codecForEncoding; }
    inline QTextCodec *codecForDecoding() const { return _codecForDecoding; }
    void setCodecForEncoding(const QString &codecName);
    void setCodecForEncoding(QTextCodec *codec);
    void setCodecForDecoding(const QString &codecName);
    void setCodecForDecoding(QTextCodec *codec);

    QString decodeString(const QByteArray &text) const;
    QByteArray encodeString(const QString &string) const;

    // only valid on client side, these are not synced!
    inline QDateTime lastChannelActivity(BufferId id) const { return _lastActivity.value(id); }
    void setLastChannelActivity(BufferId id, const QDateTime &time);
    inline QDateTime lastSpokenTo(BufferId id) const { return _lastSpokenTo.value(id); }
    void setLastSpokenTo(BufferId id, const QDateTime &time);

public slots:
    void setUser(const QString &user);
    void setHost(const QString &host);
    void setNick(const QString &nick);
    void setRealName(const QString &realName);
    /**
     * Set account name, e.g. NickServ/SASL account
     *
     * @param[in] account Account name if logged in, * if logged out, or empty string if unknown
     */
    void setAccount(const QString &account);
    void setAway(const bool &away);
    void setAwayMessage(const QString &awayMessage);
    void setIdleTime(const QDateTime &idleTime);
    void setLoginTime(const QDateTime &loginTime);
    void setServer(const QString &server);
    void setIrcOperator(const QString &ircOperator);
    void setLastAwayMessage(const QDateTime &lastAwayMessage);
    void setWhoisServiceReply(const QString &whoisServiceReply);
    void setSuserHost(const QString &suserHost);
    void setEncrypted(bool encrypted);
    void updateHostmask(const QString &mask);

    void setUserModes(const QString &modes);

    /*!
     * \brief joinChannel Called when user joins some channel, this function inserts the channel to internal list of channels this user is in.
     * \param channel Pointer to a channel this user just joined
     * \param skip_channel_join If this is false, this function will also call IrcChannel::joinIrcUser, can be set to true as a performance tweak.
     */
    void joinChannel(IrcChannel *channel, bool skip_channel_join = false);
    void joinChannel(const QString &channelname);
    void partChannel(IrcChannel *channel);
    void partChannel(const QString &channelname);
    void quit();

    void addUserModes(const QString &modes);
    void removeUserModes(const QString &modes);

signals:
//   void userSet(QString user);
//   void hostSet(QString host);
    void nickSet(QString newnick); // needed in NetworkModel
//   void realNameSet(QString realName);
    void awaySet(bool away); // needed in NetworkModel
//   void awayMessageSet(QString awayMessage);
//   void idleTimeSet(QDateTime idleTime);
//   void loginTimeSet(QDateTime loginTime);
//   void serverSet(QString server);
//   void ircOperatorSet(QString ircOperator);
//   void lastAwayMessageSet(QDateTime lastAwayMessage);
//   void whoisServiceReplySet(QString whoisServiceReply);
//   void suserHostSet(QString suserHost);
    void encryptedSet(bool encrypted);

    void userModesSet(QString modes);
    void userModesAdded(QString modes);
    void userModesRemoved(QString modes);

    // void channelJoined(QString channel);
    void channelParted(QString channel);
    void quited();

    void lastChannelActivityUpdated(BufferId id, const QDateTime &newTime);
    void lastSpokenToUpdated(BufferId id, const QDateTime &newTime);

private slots:
    void updateObjectName();
    void channelDestroyed();

private:
    inline bool operator==(const IrcUser &ircuser2)
    {
        return (_nick.toLower() == ircuser2.nick().toLower());
    }


    inline bool operator==(const QString &nickname)
    {
        return (_nick.toLower() == nickname.toLower());
    }


    bool _initialized;

    QString _nick;
    QString _user;
    QString _host;
    QString _realName;
    QString _account;      /// Account name, e.g. NickServ/SASL account
    QString _awayMessage;
    bool _away;
    QString _server;
    QDateTime _idleTime;
    QDateTime _idleTimeSet;
    QDateTime _loginTime;
    QString _ircOperator;
    QDateTime _lastAwayMessage;
    QString _whoisServiceReply;
    QString _suserHost;
    bool _encrypted;

    // QSet<QString> _channels;
    QSet<IrcChannel *> _channels;
    QString _userModes;

    Network *_network;

    QTextCodec *_codecForEncoding;
    QTextCodec *_codecForDecoding;

    QHash<BufferId, QDateTime> _lastActivity;
    QHash<BufferId, QDateTime> _lastSpokenTo;
};


#endif
