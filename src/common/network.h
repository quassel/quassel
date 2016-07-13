/***************************************************************************
 *   Copyright (C) 2005-2016 by the Quassel Project                        *
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

#ifndef NETWORK_H
#define NETWORK_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QNetworkProxy>
#include <QHash>
#include <QVariantMap>
#include <QPointer>
#include <QMutex>
#include <QByteArray>

#include "types.h"
#include "util.h"
#include "syncableobject.h"

#include "signalproxy.h"
#include "ircuser.h"
#include "ircchannel.h"

// defined below!
struct NetworkInfo;

// TODO: ConnectionInfo to propagate and sync the current state of NetworkConnection, encodings etcpp

class Network : public SyncableObject
{
    SYNCABLE_OBJECT
    Q_OBJECT
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
    //Q_PROPERTY(Network::ConnectionState connectionState READ connectionState WRITE setConnectionState)
    Q_PROPERTY(int connectionState READ connectionState WRITE setConnectionState)
    Q_PROPERTY(bool useRandomServer READ useRandomServer WRITE setUseRandomServer)
    Q_PROPERTY(QStringList perform READ perform WRITE setPerform)
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

public :
        enum ConnectionState {
        Disconnected,
        Connecting,
        Initializing,
        Initialized,
        Reconnecting,
        Disconnecting
    };

    // see:
    //  http://www.irc.org/tech_docs/005.html
    //  http://www.irc.org/tech_docs/draft-brocklesby-irc-isupport-03.txt
    enum ChannelModeType {
        NOT_A_CHANMODE = 0x00,
        A_CHANMODE = 0x01,
        B_CHANMODE = 0x02,
        C_CHANMODE = 0x04,
        D_CHANMODE = 0x08
    };

    // Default port assignments according to what many IRC networks have settled on.
    // Technically not a standard, but it's fairly widespread.
    // See https://freenode.net/news/port-6697-irc-via-tlsssl
    enum PortDefaults {
        PORT_PLAINTEXT = 6667, /// Default port for unencrypted connections
        PORT_SSL = 6697        /// Default port for encrypted connections
    };

    struct Server {
        QString host;
        uint port;
        QString password;
        bool useSsl;
        bool sslVerify;     /// If true, validate SSL certificates
        int sslVersion;

        bool useProxy;
        int proxyType;
        QString proxyHost;
        uint proxyPort;
        QString proxyUser;
        QString proxyPass;

        // sslVerify only applies when useSsl is true.  sslVerify should be enabled by default,
        // so enabling useSsl offers a more secure default.
        Server() : port(6667), useSsl(false), sslVerify(true), sslVersion(0), useProxy(false),
            proxyType(QNetworkProxy::Socks5Proxy), proxyHost("localhost"), proxyPort(8080) {}

        Server(const QString &host, uint port, const QString &password, bool useSsl,
               bool sslVerify)
            : host(host), port(port), password(password), useSsl(useSsl), sslVerify(sslVerify),
              sslVersion(0), useProxy(false), proxyType(QNetworkProxy::Socks5Proxy),
              proxyHost("localhost"), proxyPort(8080) {}

        bool operator==(const Server &other) const;
        bool operator!=(const Server &other) const;
    };
    typedef QList<Server> ServerList;

    Network(const NetworkId &networkid, QObject *parent = 0);
    ~Network();

    inline NetworkId networkId() const { return _networkId; }

    inline SignalProxy *proxy() const { return _proxy; }
    inline void setProxy(SignalProxy *proxy) { _proxy = proxy; }

    inline bool isMyNick(const QString &nick) const { return (myNick().toLower() == nick.toLower()); }
    inline bool isMe(IrcUser *ircuser) const { return (ircuser->nick().toLower() == myNick().toLower()); }

    bool isChannelName(const QString &channelname) const;

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
    bool isStatusMsg(const QString &target) const;

    inline bool isConnected() const { return _connected; }
    //Network::ConnectionState connectionState() const;
    inline int connectionState() const { return _connectionState; }

    QString prefixToMode(const QString &prefix) const;
    inline QString prefixToMode(const QCharRef &prefix) const { return prefixToMode(QString(prefix)); }
    QString modeToPrefix(const QString &mode) const;
    inline QString modeToPrefix(const QCharRef &mode) const { return modeToPrefix(QString(mode)); }

    ChannelModeType channelModeType(const QString &mode);
    inline ChannelModeType channelModeType(const QCharRef &mode) { return channelModeType(QString(mode)); }

    inline const QString &networkName() const { return _networkName; }
    inline const QString &currentServer() const { return _currentServer; }
    inline const QString &myNick() const { return _myNick; }
    inline int latency() const { return _latency; }
    inline IrcUser *me() const { return ircUser(myNick()); }
    inline IdentityId identity() const { return _identity; }
    QStringList nicks() const;
    inline QStringList channels() const { return _ircChannels.keys(); }
    /**
     * Gets the list of available capabilities.
     *
     * @returns QStringList of available capabilities
     */
    inline const QStringList caps() const { return QStringList(_caps.keys()); }
    /**
     * Gets the list of enabled (acknowledged) capabilities.
     *
     * @returns QStringList of enabled (acknowledged) capabilities
     */
    inline const QStringList capsEnabled() const { return _capsEnabled; }
    inline const ServerList &serverList() const { return _serverList; }
    inline bool useRandomServer() const { return _useRandomServer; }
    inline const QStringList &perform() const { return _perform; }
    inline bool useAutoIdentify() const { return _useAutoIdentify; }
    inline const QString &autoIdentifyService() const { return _autoIdentifyService; }
    inline const QString &autoIdentifyPassword() const { return _autoIdentifyPassword; }
    inline bool useSasl() const { return _useSasl; }
    inline const QString &saslAccount() const { return _saslAccount; }
    inline const QString &saslPassword() const { return _saslPassword; }
    inline bool useAutoReconnect() const { return _useAutoReconnect; }
    inline quint32 autoReconnectInterval() const { return _autoReconnectInterval; }
    inline quint16 autoReconnectRetries() const { return _autoReconnectRetries; }
    inline bool unlimitedReconnectRetries() const { return _unlimitedReconnectRetries; }
    inline bool rejoinChannels() const { return _rejoinChannels; }

    NetworkInfo networkInfo() const;
    void setNetworkInfo(const NetworkInfo &);

    QString prefixes() const;
    QString prefixModes() const;
    void determinePrefixes() const;

    bool supports(const QString &param) const { return _supports.contains(param); }
    QString support(const QString &param) const;

    /**
     * Checks if a given capability is acknowledged and active.
     *
     * @param[in] capability Name of capability
     * @returns True if acknowledged (active), otherwise false
     */
    inline bool capEnabled(const QString &capability) const { return _capsEnabled.contains(capability.toLower()); }
    // IRCv3 specs all use lowercase capability names

    /**
     * Gets the value of an available capability, e.g. for SASL, "EXTERNAL,PLAIN".
     *
     * @param[in] capability Name of capability
     * @returns Value of capability if one was specified, otherwise empty string
     */
    QString capValue(const QString &capability) const { return _caps.value(capability.toLower()); }
    // IRCv3 specs all use lowercase capability names
    // QHash returns the default constructed value if not found, in this case, empty string
    // See:  https://doc.qt.io/qt-4.8/qhash.html#value

    IrcUser *newIrcUser(const QString &hostmask, const QVariantMap &initData = QVariantMap());
    inline IrcUser *newIrcUser(const QByteArray &hostmask) { return newIrcUser(decodeServerString(hostmask)); }
    IrcUser *ircUser(QString nickname) const;
    inline IrcUser *ircUser(const QByteArray &nickname) const { return ircUser(decodeServerString(nickname)); }
    inline QList<IrcUser *> ircUsers() const { return _ircUsers.values(); }
    inline quint32 ircUserCount() const { return _ircUsers.count(); }

    IrcChannel *newIrcChannel(const QString &channelname, const QVariantMap &initData = QVariantMap());
    inline IrcChannel *newIrcChannel(const QByteArray &channelname) { return newIrcChannel(decodeServerString(channelname)); }
    IrcChannel *ircChannel(QString channelname) const;
    inline IrcChannel *ircChannel(const QByteArray &channelname) const { return ircChannel(decodeServerString(channelname)); }
    inline QList<IrcChannel *> ircChannels() const { return _ircChannels.values(); }
    inline quint32 ircChannelCount() const { return _ircChannels.count(); }

    QByteArray codecForServer() const;
    QByteArray codecForEncoding() const;
    QByteArray codecForDecoding() const;
    void setCodecForServer(QTextCodec *codec);
    void setCodecForEncoding(QTextCodec *codec);
    void setCodecForDecoding(QTextCodec *codec);

    QString decodeString(const QByteArray &text) const;
    QByteArray encodeString(const QString &string) const;
    QString decodeServerString(const QByteArray &text) const;
    QByteArray encodeServerString(const QString &string) const;

    static QByteArray defaultCodecForServer();
    static QByteArray defaultCodecForEncoding();
    static QByteArray defaultCodecForDecoding();
    static void setDefaultCodecForServer(const QByteArray &name);
    static void setDefaultCodecForEncoding(const QByteArray &name);
    static void setDefaultCodecForDecoding(const QByteArray &name);

    inline bool autoAwayActive() const { return _autoAwayActive; }
    inline void setAutoAwayActive(bool active) { _autoAwayActive = active; }

public slots:
    void setNetworkName(const QString &networkName);
    void setCurrentServer(const QString &currentServer);
    void setConnected(bool isConnected);
    void setConnectionState(int state);
    virtual void setMyNick(const QString &mynick);
    void setLatency(int latency);
    void setIdentity(IdentityId);

    void setServerList(const QVariantList &serverList);
    void setUseRandomServer(bool);
    void setPerform(const QStringList &);
    void setUseAutoIdentify(bool);
    void setAutoIdentifyService(const QString &);
    void setAutoIdentifyPassword(const QString &);
    void setUseSasl(bool);
    void setSaslAccount(const QString &);
    void setSaslPassword(const QString &);
    virtual void setUseAutoReconnect(bool);
    virtual void setAutoReconnectInterval(quint32);
    virtual void setAutoReconnectRetries(quint16);
    void setUnlimitedReconnectRetries(bool);
    void setRejoinChannels(bool);

    void setCodecForServer(const QByteArray &codecName);
    void setCodecForEncoding(const QByteArray &codecName);
    void setCodecForDecoding(const QByteArray &codecName);

    void addSupport(const QString &param, const QString &value = QString());
    void removeSupport(const QString &param);

    // IRCv3 capability negotiation (can be connected to signals)

    /**
     * Add an available capability, optionally providing a value.
     *
     * This may happen during first connect, or at any time later if a new capability becomes
     * available (e.g. SASL service starting).
     *
     * @param[in] capability Name of the capability
     * @param[in] value
     * @parblock
     * Optional value of the capability, e.g. sasl=plain.
     * @endparblock
     */
    void addCap(const QString &capability, const QString &value = QString());

    /**
     * Marks a capability as acknowledged (enabled by the IRC server).
     *
     * @param[in] capability Name of the capability
     */
    void acknowledgeCap(const QString &capability);

    /**
     * Removes a capability from the list of available capabilities.
     *
     * This may happen during first connect, or at any time later if an existing capability becomes
     * unavailable (e.g. SASL service stopping).  This also removes the capability from the list
     * of acknowledged capabilities.
     *
     * @param[in] capability Name of the capability
     */
    void removeCap(const QString &capability);

    /**
     * Clears all capabilities from the list of available capabilities.
     *
     * This also removes the capability from the list of acknowledged capabilities.
     */
    void clearCaps();

    inline void addIrcUser(const QString &hostmask) { newIrcUser(hostmask); }
    inline void addIrcChannel(const QString &channel) { newIrcChannel(channel); }

    //init geters
    QVariantMap initSupports() const;
    /**
     * Get the initial list of available capabilities.
     *
     * @return QVariantMap of <QString, QString> indicating available capabilities and values
     */
    QVariantMap initCaps() const;
    /**
     * Get the initial list of enabled (acknowledged) capabilities.
     *
     * @return QVariantList of QString indicating enabled (acknowledged) capabilities and values
     */
    QVariantList initCapsEnabled() const { return toVariantList(capsEnabled()); }
    inline QVariantList initServerList() const { return toVariantList(serverList()); }
    virtual QVariantMap initIrcUsersAndChannels() const;

    //init seters
    void initSetSupports(const QVariantMap &supports);
    /**
     * Initialize the list of available capabilities.
     *
     * @param[in] caps QVariantMap of <QString, QString> indicating available capabilities and values
     */
    void initSetCaps(const QVariantMap &caps);
    /**
     * Initialize the list of enabled (acknowledged) capabilities.
     *
     * @param[in] caps QVariantList of QString indicating enabled (acknowledged) capabilities and values
     */
    inline void initSetCapsEnabled(const QVariantList &capsEnabled) { _capsEnabled = fromVariantList<QString>(capsEnabled); }
    inline void initSetServerList(const QVariantList &serverList) { _serverList = fromVariantList<Server>(serverList); }
    virtual void initSetIrcUsersAndChannels(const QVariantMap &usersAndChannels);

    /**
     * Update IrcUser hostmask and username from mask, creating an IrcUser if one does not exist.
     *
     * @param[in] mask   Full nick!user@hostmask string
     * @return IrcUser of the matching nick if exists, otherwise a new IrcUser
     */
    IrcUser *updateNickFromMask(const QString &mask);

    // these slots are to keep the hashlists of all users and the
    // channel lists up to date
    void ircUserNickChanged(QString newnick);

    virtual inline void requestConnect() const { REQUEST(NO_ARG) }
    virtual inline void requestDisconnect() const { REQUEST(NO_ARG) }
    virtual inline void requestSetNetworkInfo(const NetworkInfo &info) { REQUEST(ARG(info)) }

    void emitConnectionError(const QString &);

protected slots:
    virtual void removeIrcUser(IrcUser *ircuser);
    virtual void removeIrcChannel(IrcChannel *ircChannel);
    virtual void removeChansAndUsers();

signals:
    void aboutToBeDestroyed();
    void networkNameSet(const QString &networkName);
    void currentServerSet(const QString &currentServer);
    void connectedSet(bool isConnected);
    void connectionStateSet(Network::ConnectionState);
//   void connectionStateSet(int);
    void connectionError(const QString &errorMsg);
    void myNickSet(const QString &mynick);
//   void latencySet(int latency);
    void identitySet(IdentityId);

    void configChanged();

    //   void serverListSet(QVariantList serverList);
//   void useRandomServerSet(bool);
//   void performSet(const QStringList &);
//   void useAutoIdentifySet(bool);
//   void autoIdentifyServiceSet(const QString &);
//   void autoIdentifyPasswordSet(const QString &);
//   void useAutoReconnectSet(bool);
//   void autoReconnectIntervalSet(quint32);
//   void autoReconnectRetriesSet(quint16);
//   void unlimitedReconnectRetriesSet(bool);
//   void rejoinChannelsSet(bool);

//   void codecForServerSet(const QByteArray &codecName);
//   void codecForEncodingSet(const QByteArray &codecName);
//   void codecForDecodingSet(const QByteArray &codecName);

//   void supportAdded(const QString &param, const QString &value);
//   void supportRemoved(const QString &param);

    // IRCv3 capability negotiation (can drive other slots)
    /**
     * Indicates a capability is now available, with optional value in Network::capValue().
     *
     * @see Network::addCap()
     *
     * @param[in] capability Name of the capability
     */
    void capAdded (const QString &capability);

    /**
     * Indicates a capability was acknowledged (enabled by the IRC server).
     *
     * @see Network::acknowledgeCap()
     *
     * @param[in] capability Name of the capability
     */
    void capAcknowledged(const QString &capability);

    /**
     * Indicates a capability was removed from the list of available capabilities.
     *
     * @see Network::removeCap()
     *
     * @param[in] capability Name of the capability
     */
    void capRemoved(const QString &capability);

//   void ircUserAdded(const QString &hostmask);
    void ircUserAdded(IrcUser *);
//   void ircChannelAdded(const QString &channelname);
    void ircChannelAdded(IrcChannel *);

//   void connectRequested() const;
//   void disconnectRequested() const;
//   void setNetworkInfoRequested(const NetworkInfo &) const;

protected:
    inline virtual IrcChannel *ircChannelFactory(const QString &channelname) { return new IrcChannel(channelname, this); }
    inline virtual IrcUser *ircUserFactory(const QString &hostmask) { return new IrcUser(hostmask, this); }

private:
    QPointer<SignalProxy> _proxy;

    NetworkId _networkId;
    IdentityId _identity;

    QString _myNick;
    int _latency;
    QString _networkName;
    QString _currentServer;
    bool _connected;
    ConnectionState _connectionState;

    mutable QString _prefixes;
    mutable QString _prefixModes;

    QHash<QString, IrcUser *> _ircUsers; // stores all known nicks for the server
    QHash<QString, IrcChannel *> _ircChannels; // stores all known channels
    QHash<QString, QString> _supports; // stores results from RPL_ISUPPORT

    QHash<QString, QString> _caps;  /// Capabilities supported by the IRC server
    // By synchronizing the supported capabilities, the client could suggest certain behaviors, e.g.
    // in the Network settings dialog, recommending SASL instead of using NickServ, or warning if
    // SASL EXTERNAL isn't available.
    QStringList _capsEnabled;       /// Enabled capabilities that received 'CAP ACK'
    // _capsEnabled uses the same values from the <name>=<value> pairs stored in _caps

    ServerList _serverList;
    bool _useRandomServer;
    QStringList _perform;

    bool _useAutoIdentify;
    QString _autoIdentifyService;
    QString _autoIdentifyPassword;

    bool _useSasl;
    QString _saslAccount;
    QString _saslPassword;

    bool _useAutoReconnect;
    quint32 _autoReconnectInterval;
    quint16 _autoReconnectRetries;
    bool _unlimitedReconnectRetries;
    bool _rejoinChannels;

    QTextCodec *_codecForServer;
    QTextCodec *_codecForEncoding;
    QTextCodec *_codecForDecoding;

    static QTextCodec *_defaultCodecForServer;
    static QTextCodec *_defaultCodecForEncoding;
    static QTextCodec *_defaultCodecForDecoding;

    bool _autoAwayActive; // when this is active handle305 and handle306 don't trigger any output

    friend class IrcUser;
    friend class IrcChannel;
};


//! Stores all editable information about a network (as opposed to runtime state).
struct NetworkInfo {
    // set some default values, note that this does not initialize e.g. name and id
    NetworkInfo();

    NetworkId networkId;
    QString networkName;
    IdentityId identity;

    bool useCustomEncodings; // not used!
    QByteArray codecForServer;
    QByteArray codecForEncoding;
    QByteArray codecForDecoding;

    Network::ServerList serverList;
    bool useRandomServer;

    QStringList perform;

    bool useAutoIdentify;
    QString autoIdentifyService;
    QString autoIdentifyPassword;

    bool useSasl;
    QString saslAccount;
    QString saslPassword;

    bool useAutoReconnect;
    quint32 autoReconnectInterval;
    quint16 autoReconnectRetries;
    bool unlimitedReconnectRetries;
    bool rejoinChannels;

    bool operator==(const NetworkInfo &other) const;
    bool operator!=(const NetworkInfo &other) const;
};

QDataStream &operator<<(QDataStream &out, const NetworkInfo &info);
QDataStream &operator>>(QDataStream &in, NetworkInfo &info);
QDebug operator<<(QDebug dbg, const NetworkInfo &i);
Q_DECLARE_METATYPE(NetworkInfo)

QDataStream &operator<<(QDataStream &out, const Network::Server &server);
QDataStream &operator>>(QDataStream &in, Network::Server &server);
QDebug operator<<(QDebug dbg, const Network::Server &server);
Q_DECLARE_METATYPE(Network::Server)

#endif
