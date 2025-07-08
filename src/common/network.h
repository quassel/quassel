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

#include "common-export.h"

#include <optional>
#include <utility>

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QMutex>
#include <QNetworkProxy>
#include <QPointer>
#include <QString>
#include <QStringConverter>
#include <QStringList>
#include <QVariantMap>

#include "ircchannel.h"
#include "ircuser.h"
#include "signalproxy.h"
#include "syncableobject.h"
#include "types.h"
#include "util.h"

// IRCv3 capabilities
#include "irccap.h"

// Forward declaration of NetworkInfo
struct NetworkInfo;

class COMMON_EXPORT Network : public SyncableObject
{
    Q_OBJECT
    SYNCABLE_OBJECT

    Q_ENUMS(ConnectionState)

    Q_PROPERTY(QString networkName READ networkName WRITE setNetworkName)
    Q_PROPERTY(QString currentServer READ currentServer WRITE setCurrentServer)
    Q_PROPERTY(QString myNick READ myNick WRITE setMyNick)
    Q_PROPERTY(int latency READ latency WRITE setLatency)
    Q_PROPERTY(QByteArray codecForServer READ codecForServer WRITE setCodecForServer)
    Q_PROPERTY(QByteArray codecForEncoding READ codecForEncoding WRITE setCodecForEncoding)
    Q_PROPERTY(QByteArray codecForDecoding READ codecForDecoding WRITE setCodecForDecoding)
    Q_PROPERTY(IdentityId identityId READ identity WRITE setIdentity)
    Q_PROPERTY(bool isConnected READ isConnected WRITE setConnected)
    Q_PROPERTY(int connectionState READ connectionState WRITE setConnectionState)
    Q_PROPERTY(bool useRandomServer READ useRandomServer WRITE setUseRandomServer)
    Q_PROPERTY(QStringList perform READ perform WRITE setPerform)
    Q_PROPERTY(QStringList skipCaps READ skipCaps WRITE setSkipCaps)
    Q_PROPERTY(bool useAutoIdentify READ useAutoIdentify WRITE setUseAutoIdentify)
    Q_PROPERTY(QString autoIdentifyService READ autoIdentifyService WRITE setAutoIdentifyService)
    Q_PROPERTY(QString autoIdentifyPassword READ autoIdentifyPassword WRITE setAutoIdentifyPassword)
    Q_PROPERTY(bool useSasl READ useSasl WRITE setUseSasl)
    Q_PROPERTY(QString saslAccount READ saslAccount WRITE setSaslAccount)
    Q_PROPERTY(QString saslPassword READ saslPassword WRITE setSaslPassword)
    Q_PROPERTY(bool useAutoReconnect READ useAutoReconnect WRITE setUseAutoReconnect)
    Q_PROPERTY(quint32 autoReconnectInterval READ autoReconnectInterval WRITE setAutoReconnectInterval)
    Q_PROPERTY(quint16 autoReconnectRetries READ autoReconnectRetries WRITE setAutoReconnectRetries)
    Q_PROPERTY(bool unlimitedReconnectRetries READ unlimitedReconnectRetries WRITE setUnlimitedReconnectRetries)
    Q_PROPERTY(bool rejoinChannels READ rejoinChannels WRITE setRejoinChannels)
    // Custom rate limiting
    Q_PROPERTY(bool useCustomMessageRate READ useCustomMessageRate WRITE setUseCustomMessageRate)
    Q_PROPERTY(quint32 msgRateBurstSize READ messageRateBurstSize WRITE setMessageRateBurstSize)
    Q_PROPERTY(quint32 msgRateMessageDelay READ messageRateDelay WRITE setMessageRateDelay)
    Q_PROPERTY(bool unlimitedMessageRate READ unlimitedMessageRate WRITE setUnlimitedMessageRate)

public:
    enum ConnectionState
    {
        Disconnected,
        Connecting,
        Initializing,
        Initialized,
        Reconnecting,
        Disconnecting
    };

    enum ChannelModeType
    {
        NOT_A_CHANMODE = 0x00,
        A_CHANMODE = 0x01,
        B_CHANMODE = 0x02,
        C_CHANMODE = 0x04,
        D_CHANMODE = 0x08
    };

    /// Default port assignments according to what many IRC networks have settled on.
    /// Technically not a standard, but it's fairly widespread.
    /// See https://freenode.net/news/port-6697-irc-via-tlsssl
    enum PortDefaults
    {
        PORT_PLAINTEXT = 6667,  ///< Default port for unencrypted connections
        PORT_SSL = 6697         ///< Default port for encrypted connections
    };

    struct Server
    {
        QString host;
        uint port{PortDefaults::PORT_SSL};
        QString password;
        bool useSsl{true};     ///< If true, connect via SSL/TLS
        bool sslVerify{true};  ///< If true, validate SSL certificates
        int sslVersion{0};

        bool useProxy{false};
        int proxyType{QNetworkProxy::Socks5Proxy};
        QString proxyHost;
        uint proxyPort{8080};
        QString proxyUser;
        QString proxyPass;

        // sslVerify only applies when useSsl is true.  sslVerify should be enabled by default,
        // so enabling useSsl offers a more secure default.
        Server()
            : proxyHost("localhost")
        {
        }

        Server(QString host, uint port, QString password, bool useSsl, bool sslVerify)
            : host(std::move(host))
            , port(port)
            , password(std::move(password))
            , useSsl(useSsl)
            , sslVerify(sslVerify)
            , proxyType(QNetworkProxy::Socks5Proxy)
            , proxyHost("localhost")
            , proxyPort(8080)
        {
        }

        bool operator==(const Server& other) const;
        bool operator!=(const Server& other) const;
    };
    using ServerList = QList<Server>;

    Network(const NetworkId& networkid, QObject* parent = nullptr);
    ~Network() override;

    inline NetworkId networkId() const { return _networkId; }

    inline SignalProxy* proxy() const { return _proxy; }
    inline void setProxy(SignalProxy* proxy) { _proxy = proxy; }

    inline bool isMyNick(const QString& nick) const { return (myNick().toLower() == nick.toLower()); }
    inline bool isMe(IrcUser* ircuser) const { return (ircuser->nick().toLower() == myNick().toLower()); }

    bool isChannelName(const QString& channelname) const;

    /**
     * Checks if the target counts as a STATUSMSG
     *
     * Status messages are prefixed with one or more characters from the server-provided STATUSMSG
     * if available, otherwise "@" and "+" are assumed.  Generally, status messages sent to a
     * channel are only visible to those with the same or higher permissions, e.g. voiced.
     *
     * @param[in] target Name of destination, e.g. a channel or query
     * @returns True if a STATUSMSG, otherwise false
     */
    bool isStatusMsg(const QString& target) const;

    inline bool isConnected() const { return _connected; }
    inline int connectionState() const { return _connectionState; }

    /**@{*/
    /**
     * Translates a user’s prefix to the channelmode associated with it.
     * @param prefix Prefix to be translated.
     */
    QString prefixToMode(const QString& prefix) const;
    inline QString prefixToMode(const QChar& prefix) const { return prefixToMode(QString(prefix)); }
    inline QString prefixesToModes(const QString& prefix) const
    {
        QString modes;
        for (QChar c : prefix) {
            modes += prefixToMode(c);
        }
        return modes;
    }
    /**@}*/

    /**@{*/
    /**
     * Translates a user’s mode to the prefix associated with it.
     * @param mode Mode to be translated.
     */
    QString modeToPrefix(const QString& mode) const;
    inline QString modeToPrefix(const QChar& mode) const { return modeToPrefix(QString(mode)); }
    inline QString modesToPrefixes(const QString& mode) const
    {
        QString prefixes;
        for (QChar c : mode) {
            prefixes += modeToPrefix(c);
        }
        return prefixes;
    }
    /**@}*/

    /**
     * Sorts the user channelmodes according to priority set by PREFIX
     *
     * Given a list of channel modes, sorts according to the order of PREFIX, putting the highest
     * modes first.  Any unknown modes are moved to the end in no given order.
     *
     * If prefix modes cannot be determined from the network, no changes will be made.
     *
     * @param modes User channelmodes
     * @return Priority-sorted user channelmodes
     */
    QString sortPrefixModes(const QString& modes) const;

    /**@{*/
    /**
     * Sorts the list of users' channelmodes according to priority set by PREFIX
     *
     * Maintains order of the modes list.
     *
     * @seealso Network::sortPrefixModes()
     *
     * @param modesList List of users' channel modes
     * @return Priority-sorted list of users' channel modes
     */
    inline QStringList sortPrefixModes(const QStringList& modesList) const
    {
        QStringList sortedModesList;
        // Sort each individual mode string, appending back
        // Must maintain the order received!
        for (QString modes : modesList) {
            sortedModesList << sortPrefixModes(modes);
        }
        return sortedModesList;
    }
    /**@}*/

    ChannelModeType channelModeType(const QString& mode) const;
    inline ChannelModeType channelModeType(const QChar& mode) const { return channelModeType(QString(mode)); }

    inline QStringList channelPrefixes() const { return _channelPrefixes; }
    inline QStringList prefixModes() const { return _prefixModes; }
    inline QStringList prefixes() const { return _prefixes; }
    inline QString statusMsg() const { return _statusMsg; }

    inline QString networkName() const { return _networkName; }
    inline QString currentServer() const { return _currentServer; }
    inline QString myNick() const { return _myNick; }
    inline int latency() const { return _latency; }
    inline IdentityId identity() const { return _identity; }
    NetworkInfo toNetworkInfo() const;
    inline ServerList serverList() const { return _serverList; }
    inline bool useRandomServer() const { return _useRandomServer; }
    inline QStringList perform() const { return _perform; }
    inline QStringList skipCaps() const { return _skipCaps; }
    inline bool useAutoIdentify() const { return _useAutoIdentify; }
    inline QString autoIdentifyService() const { return _autoIdentifyService; }
    inline QString autoIdentifyPassword() const { return _autoIdentifyPassword; }
    inline bool useSasl() const { return _useSasl; }
    inline QString saslAccount() const { return _saslAccount; }
    inline QString saslPassword() const { return _saslPassword; }
    inline bool useAutoReconnect() const { return _useAutoReconnect; }
    inline quint32 autoReconnectInterval() const { return _autoReconnectInterval; }
    inline quint16 autoReconnectRetries() const { return _autoReconnectRetries; }
    inline bool unlimitedReconnectRetries() const { return _unlimitedReconnectRetries; }
    inline bool rejoinChannels() const { return _rejoinChannels; }
    inline bool useCustomMessageRate() const { return _useCustomMessageRate; }
    inline quint32 messageRateBurstSize() const { return _messageRateBurstSize; }
    inline quint32 messageRateDelay() const { return _messageRateDelay; }
    inline bool unlimitedMessageRate() const { return _unlimitedMessageRate; }
    inline const QHash<QString, QString>& supports() const { return _supports; }
    inline const QString& capNegotiationStatus() const { return _capNegotiationStatus; }
    inline const QStringList& availableCaps() const { return _availableCaps; }
    inline const QStringList& enabledCaps() const { return _enabledCaps; }

    IrcUser* ircUser(const QString& nickname) const;
    inline const QHash<QString, IrcUser*>* ircUsers() const { return &_ircUsers; }
    IrcChannel* ircChannel(const QString& channelname) const;
    inline const QHash<QString, IrcChannel*>* ircChannels() const { return &_ircChannels; }

    IrcUser* newIrcUser(const QString& hostmask, const QVariantMap& initData = QVariantMap());
    IrcChannel* newIrcChannel(const QString& channelname, const QVariantMap& initData = QVariantMap());

    void removeIrcUser(IrcUser* ircuser);
    void removeIrcChannel(IrcChannel* ircChannel);
    void removeChansAndUsers();

    QVariantMap initIrcUsersAndChannels() const;
    void initSetIrcUsersAndChannels(const QVariantMap& usersAndChannels);

    inline bool autoAwayActive() const { return _autoAwayActive; }
    inline QDateTime lastAwayMessageTime() const { return _lastAwayMessageTime; }

    // codec stuff
    QByteArray codecForServer() const;
    QByteArray codecForEncoding() const;
    QByteArray codecForDecoding() const;

    static QByteArray defaultCodecForServer();
    static QByteArray defaultCodecForEncoding();
    static QByteArray defaultCodecForDecoding();

public slots:
    void setNetworkName(const QString& networkName);
    void setCurrentServer(const QString& currentServer);
    void setMyNick(const QString& mynick);
    void setLatency(int latency);
    void setCodecForServer(const QByteArray& codecName);
    void setCodecForEncoding(const QByteArray& codecName);
    void setCodecForDecoding(const QByteArray& codecName);
    void setIdentity(IdentityId identity);
    void setConnected(bool connected);
    void setConnectionState(int state);
    void setServerList(const Network::ServerList& serverList);
    void setUseRandomServer(bool useRandomServer);
    void setPerform(const QStringList& perform);
    void setSkipCaps(const QStringList& skipCaps);
    void setUseAutoIdentify(bool useAutoIdentify);
    void setAutoIdentifyService(const QString& autoIdentifyService);
    void setAutoIdentifyPassword(const QString& autoIdentifyPassword);
    void setUseSasl(bool useSasl);
    void setSaslAccount(const QString& saslAccount);
    void setSaslPassword(const QString& saslPassword);
    void setUseAutoReconnect(bool useAutoReconnect);
    void setAutoReconnectInterval(quint32 autoReconnectInterval);
    void setAutoReconnectRetries(quint16 autoReconnectRetries);
    void setUnlimitedReconnectRetries(bool unlimitedReconnectRetries);
    void setRejoinChannels(bool rejoinChannels);
    void setUseCustomMessageRate(bool useCustomMessageRate);
    void setMessageRateBurstSize(quint32 messageRateBurstSize);
    void setMessageRateDelay(quint32 messageRateDelay);
    void setUnlimitedMessageRate(bool unlimitedMessageRate);

    static void setDefaultCodecForServer(const QByteArray& name);
    static void setDefaultCodecForEncoding(const QByteArray& name);
    static void setDefaultCodecForDecoding(const QByteArray& name);

    QByteArray encodeString(const QString& string) const;
    QString decodeString(const QByteArray& string) const;
    QByteArray encodeServerString(const QString& string) const;
    QString decodeServerString(const QByteArray& string) const;

    void setChannelPrefixes(const QString& prefixes);
    void setPrefixModes(const QString& modes);
    void setPrefixes(const QString& modes, const QString& prefixes);
    void setStatusMsg(const QString& statusMsg);
    void addSupport(const QString& param, const QString& value = QString());
    void removeSupport(const QString& param);
    void setCapNegotiationStatus(const QString& status);
    void addCap(const QString& capability, const QString& value = QString());
    void acknowledgeCap(const QString& capability);
    void removeCap(const QString& capability);
    void clearCaps();

    void updateAutoAway(bool active, const QDateTime& lastAwayMessageTime = QDateTime());

signals:
    void configChanged();
    void connected();
    void disconnected();

    void connectionStateSet(Network::ConnectionState);
    void myNickSet(const QString& nick);

    void newIrcUserSynced(IrcUser* ircuser);
    void ircUserRemoved(IrcUser* ircuser);
    void ircChannelRemoved(IrcChannel* ircChannel);

private slots:
    void ircUserNickSet(const QString& newnick);
    void ircUserDestroyed();

private:
    NetworkId _networkId;
    SignalProxy* _proxy{nullptr};

    QString _networkName;
    QString _currentServer;
    QString _myNick;
    int _latency{0};
    IdentityId _identity;
    bool _connected{false};
    int _connectionState{Disconnected};
    ServerList _serverList;
    bool _useRandomServer{false};
    QStringList _perform;
    QStringList _skipCaps;
    bool _useAutoIdentify{false};
    QString _autoIdentifyService;
    QString _autoIdentifyPassword;
    bool _useSasl{false};
    QString _saslAccount;
    QString _saslPassword;
    bool _useAutoReconnect{true};
    quint32 _autoReconnectInterval{60};
    quint16 _autoReconnectRetries{20};
    bool _unlimitedReconnectRetries{false};
    bool _rejoinChannels{true};
    bool _useCustomMessageRate{false};
    quint32 _messageRateBurstSize{5};
    quint32 _messageRateDelay{2200};
    bool _unlimitedMessageRate{false};
    bool _autoAwayActive{false};
    QDateTime _lastAwayMessageTime;

    QHash<QString, IrcUser*> _ircUsers;
    QHash<QString, IrcChannel*> _ircChannels;

    QStringList _channelPrefixes;
    QStringList _prefixModes;
    QStringList _prefixes;
    QString _statusMsg;

    QHash<QString, QString> _supports;
    QString _capNegotiationStatus;
    QStringList _availableCaps;
    QStringList _enabledCaps;

    // encoding stuff
    std::optional<QStringConverter::Encoding> _codecForServer;
    std::optional<QStringConverter::Encoding> _codecForEncoding;
    std::optional<QStringConverter::Encoding> _codecForDecoding;

    static std::optional<QStringConverter::Encoding> _defaultCodecForServer;
    static std::optional<QStringConverter::Encoding> _defaultCodecForEncoding;
    static std::optional<QStringConverter::Encoding> _defaultCodecForDecoding;

    mutable QMutex _capsMutex;
};

QDataStream& operator<<(QDataStream& out, const NetworkInfo& info);
QDataStream& operator>>(QDataStream& in, NetworkInfo& info);
QDataStream& operator<<(QDataStream& out, const Network::Server& server);
QDataStream& operator>>(QDataStream& in, Network::Server& server);

struct NetworkInfo
{
    NetworkId networkId;
    QString networkName;
    IdentityId identity;
    QByteArray codecForServer;
    QByteArray codecForEncoding;
    QByteArray codecForDecoding;
    Network::ServerList serverList;
    bool useRandomServer{false};
    QStringList perform;
    QStringList skipCaps;
    bool useAutoIdentify{false};
    QString autoIdentifyService;
    QString autoIdentifyPassword;
    bool useSasl{false};
    QString saslAccount;
    QString saslPassword;
    bool useAutoReconnect{true};
    quint32 autoReconnectInterval{60};
    quint16 autoReconnectRetries{20};
    bool unlimitedReconnectRetries{false};
    bool rejoinChannels{true};
    bool useCustomMessageRate{false};
    quint32 messageRateBurstSize{5};
    quint32 messageRateDelay{2200};
    bool unlimitedMessageRate{false};

    QVariantMap toVariantMap() const;
    void fromVariantMap(const QVariantMap& map);
};

// Comparison operators for NetworkInfo
inline bool operator==(const NetworkInfo& lhs, const NetworkInfo& rhs)
{
    return lhs.networkId == rhs.networkId && lhs.networkName == rhs.networkName && lhs.identity == rhs.identity
           && lhs.codecForServer == rhs.codecForServer && lhs.codecForEncoding == rhs.codecForEncoding
           && lhs.codecForDecoding == rhs.codecForDecoding && lhs.serverList == rhs.serverList && lhs.useRandomServer == rhs.useRandomServer
           && lhs.perform == rhs.perform && lhs.skipCaps == rhs.skipCaps && lhs.useAutoIdentify == rhs.useAutoIdentify
           && lhs.autoIdentifyService == rhs.autoIdentifyService && lhs.autoIdentifyPassword == rhs.autoIdentifyPassword
           && lhs.useSasl == rhs.useSasl && lhs.saslAccount == rhs.saslAccount && lhs.saslPassword == rhs.saslPassword
           && lhs.useAutoReconnect == rhs.useAutoReconnect && lhs.autoReconnectInterval == rhs.autoReconnectInterval
           && lhs.autoReconnectRetries == rhs.autoReconnectRetries && lhs.unlimitedReconnectRetries == rhs.unlimitedReconnectRetries
           && lhs.rejoinChannels == rhs.rejoinChannels && lhs.useCustomMessageRate == rhs.useCustomMessageRate
           && lhs.messageRateBurstSize == rhs.messageRateBurstSize && lhs.messageRateDelay == rhs.messageRateDelay
           && lhs.unlimitedMessageRate == rhs.unlimitedMessageRate;
}

inline bool operator!=(const NetworkInfo& lhs, const NetworkInfo& rhs)
{
    return !(lhs == rhs);
}

Q_DECLARE_METATYPE(NetworkInfo)
Q_DECLARE_METATYPE(Network::Server)
