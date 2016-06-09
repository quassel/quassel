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

#ifndef CORENETWORK_H
#define CORENETWORK_H

#include "network.h"
#include "coreircchannel.h"
#include "coreircuser.h"

// IRCv3 capabilities
#include "irccap.h"

#include <QTimer>

#ifdef HAVE_SSL
# include <QSslSocket>
# include <QSslError>
#else
# include <QTcpSocket>
#endif

#ifdef HAVE_QCA2
#  include "cipher.h"
#endif

#include "coresession.h"

#include <functional>

class CoreIdentity;
class CoreUserInputHandler;
class CoreIgnoreListManager;
class Event;

class CoreNetwork : public Network
{
    SYNCABLE_OBJECT
        Q_OBJECT

public:
    CoreNetwork(const NetworkId &networkid, CoreSession *session);
    ~CoreNetwork();
    inline virtual const QMetaObject *syncMetaObject() const { return &Network::staticMetaObject; }

    inline CoreIdentity *identityPtr() const { return coreSession()->identity(identity()); }
    inline CoreSession *coreSession() const { return _coreSession; }
    inline CoreNetworkConfig *networkConfig() const { return coreSession()->networkConfig(); }

    inline CoreUserInputHandler *userInputHandler() const { return _userInputHandler; }
    inline CoreIgnoreListManager *ignoreListManager() { return coreSession()->ignoreListManager(); }

    //! Decode a string using the server (network) decoding.
    inline QString serverDecode(const QByteArray &string) const { return decodeServerString(string); }

    //! Decode a string using a channel-specific encoding if one is set (and use the standard encoding else).
    QString channelDecode(const QString &channelName, const QByteArray &string) const;

    //! Decode a string using an IrcUser-specific encoding, if one exists (using the standaed encoding else).
    QString userDecode(const QString &userNick, const QByteArray &string) const;

    //! Encode a string using the server (network) encoding.
    inline QByteArray serverEncode(const QString &string) const { return encodeServerString(string); }

    //! Encode a string using the channel-specific encoding, if set, and use the standard encoding else.
    QByteArray channelEncode(const QString &channelName, const QString &string) const;

    //! Encode a string using the user-specific encoding, if set, and use the standard encoding else.
    QByteArray userEncode(const QString &userNick, const QString &string) const;

    inline QString channelKey(const QString &channel) const { return _channelKeys.value(channel.toLower(), QString()); }

    inline QByteArray readChannelCipherKey(const QString &channel) const { return _cipherKeys.value(channel.toLower()); }
    inline void storeChannelCipherKey(const QString &channel, const QByteArray &key) { _cipherKeys[channel.toLower()] = key; }

    inline bool isAutoWhoInProgress(const QString &channel) const { return _autoWhoPending.value(channel.toLower(), 0); }

    inline UserId userId() const { return _coreSession->user(); }

    inline QAbstractSocket::SocketState socketState() const { return socket.state(); }
    inline bool socketConnected() const { return socket.state() == QAbstractSocket::ConnectedState; }
    inline QHostAddress localAddress() const { return socket.localAddress(); }
    inline QHostAddress peerAddress() const { return socket.peerAddress(); }
    inline quint16 localPort() const { return socket.localPort(); }
    inline quint16 peerPort() const { return socket.peerPort(); }

    QList<QList<QByteArray>> splitMessage(const QString &cmd, const QString &message, std::function<QList<QByteArray>(QString &)> cmdGenerator);

    // IRCv3 capability negotiation

    /**
     * Checks if capability negotiation is currently ongoing.
     *
     * @returns True if in progress, otherwise false
     */
    inline bool capNegotiationInProgress() const { return !_capsQueued.empty(); }

    /**
     * Queues a capability to be requested.
     *
     * Adds to the list of capabilities being requested.  If non-empty, CAP REQ messages are sent
     * to the IRC server.  This may happen at login or if capabilities are announced via CAP NEW.
     *
     * @param[in] capability Name of the capability
     */
    void queueCap(const QString &capability);

    /**
     * Begins capability negotiation if capabilities are queued, otherwise returns.
     *
     * If any capabilities are queued, this will begin the cycle of taking each capability and
     * requesting it.  When no capabilities remain, capability negotiation is suitably ended.
     */
    void beginCapNegotiation();

    /**
     * List of capabilities requiring further core<->server messages to configure.
     *
     * For example, SASL requires the back-and-forth of AUTHENTICATE, so the next capability cannot
     * be immediately sent.
     *
     * See: http://ircv3.net/specs/extensions/sasl-3.2.html
     */
    const QStringList capsRequiringConfiguration = QStringList {
        IrcCap::SASL
    };

public slots:
    virtual void setMyNick(const QString &mynick);

    virtual void requestConnect() const;
    virtual void requestDisconnect() const;
    virtual void requestSetNetworkInfo(const NetworkInfo &info);

    virtual void setUseAutoReconnect(bool);
    virtual void setAutoReconnectInterval(quint32);
    virtual void setAutoReconnectRetries(quint16);

    void setPingInterval(int interval);

    void connectToIrc(bool reconnecting = false);
    /**
     * Disconnect from the IRC server.
     *
     * Begin disconnecting from the IRC server, including optionally reconnecting.
     *
     * @param requested       If true, user requested this disconnect; don't try to reconnect
     * @param reason          Reason for quitting, defaulting to the user-configured quit reason
     * @param withReconnect   Reconnect to the network after disconnecting (e.g. ping timeout)
     * @param forceImmediate  Immediately disconnect from network, skipping queue of other commands
     */
    void disconnectFromIrc(bool requested = true, const QString &reason = QString(),
                           bool withReconnect = false, bool forceImmediate = false);

    void userInput(BufferInfo bufferInfo, QString msg);

    /**
     * Sends the raw (encoded) line, adding to the queue if needed, optionally with higher priority.
     *
     * @param[in] input   QByteArray of encoded characters
     * @param[in] prepend
     * @parmblock
     * If true, the line is prepended into the start of the queue, otherwise, it's appended to the
     * end.  This should be used sparingly, for if either the core or the IRC server cannot maintain
     * PING/PONG replies, the other side will close the connection.
     * @endparmblock
     */
    void putRawLine(const QByteArray input, const bool prepend = false);

    /**
     * Sends the command with encoded parameters, with optional prefix or high priority.
     *
     * @param[in] cmd      Command to send, ignoring capitalization
     * @param[in] params   Parameters for the command, encoded within a QByteArray
     * @param[in] prefix   Optional command prefix
     * @param[in] prepend
     * @parmblock
     * If true, the command is prepended into the start of the queue, otherwise, it's appended to
     * the end.  This should be used sparingly, for if either the core or the IRC server cannot
     * maintain PING/PONG replies, the other side will close the connection.
     * @endparmblock
     */
    void putCmd(const QString &cmd, const QList<QByteArray> &params, const QByteArray &prefix = QByteArray(), const bool prepend = false);

    /**
     * Sends the command for each set of encoded parameters, with optional prefix or high priority.
     *
     * @param[in] cmd         Command to send, ignoring capitalization
     * @param[in] params
     * @parmblock
     * List of parameter lists for the command, encoded within a QByteArray.  The command will be
     * sent multiple times, once for each set of params stored within the outer list.
     * @endparmblock
     * @param[in] prefix      Optional command prefix
     * @param[in] prependAll
     * @parmblock
     * If true, ALL of the commands are prepended into the start of the queue, otherwise, they're
     * appended to the end.  This should be used sparingly, for if either the core or the IRC server
     * cannot maintain PING/PONG replies, the other side will close the connection.
     * @endparmblock
     */
    void putCmd(const QString &cmd, const QList<QList<QByteArray>> &params, const QByteArray &prefix = QByteArray(), const bool prependAll = false);

    void setChannelJoined(const QString &channel);
    void setChannelParted(const QString &channel);
    void addChannelKey(const QString &channel, const QString &key);
    void removeChannelKey(const QString &channel);

    // Blowfish stuff
#ifdef HAVE_QCA2
    Cipher *cipher(const QString &recipient);
    QByteArray cipherKey(const QString &recipient) const;
    void setCipherKey(const QString &recipient, const QByteArray &key);
    bool cipherUsesCBC(const QString &target);
#endif

    // IRCv3 capability negotiation (can be connected to signals)

    /**
     * Indicates a capability is now available, with optional value in Network::capValue().
     *
     * @see Network::addCap()
     *
     * @param[in] capability Name of the capability
     */
    void serverCapAdded(const QString &capability);

    /**
     * Indicates a capability was acknowledged (enabled by the IRC server).
     *
     * @see Network::acknowledgeCap()
     *
     * @param[in] capability Name of the capability
     */
    void serverCapAcknowledged(const QString &capability);

    /**
     * Indicates a capability was removed from the list of available capabilities.
     *
     * @see Network::removeCap()
     *
     * @param[in] capability Name of the capability
     */
    void serverCapRemoved(const QString &capability);

    /**
     * Sends the next capability from the queue.
     *
     * During nick registration if any capabilities remain queued, this will take the next and
     * request it.  When no capabilities remain, capability negotiation is ended.
     */
    void sendNextCap();

    void setAutoWhoEnabled(bool enabled);
    void setAutoWhoInterval(int interval);
    void setAutoWhoDelay(int delay);

    /**
     * Appends the given channel/nick to the front of the AutoWho queue.
     *
     * When 'away-notify' is enabled, this will trigger an immediate AutoWho since regular
     * who-cycles are disabled as per IRCv3 specifications.
     *
     * @param[in] channelOrNick Channel or nickname to WHO
     */
    void queueAutoWhoOneshot(const QString &channelOrNick);

    bool setAutoWhoDone(const QString &channel);

    void updateIssuedModes(const QString &requestedModes);
    void updatePersistentModes(QString addModes, QString removeModes);
    void resetPersistentModes();

    Server usedServer() const;

    inline void resetPingTimeout() { _pingCount = 0; }

    inline void displayMsg(Message::Type msgType, BufferInfo::Type bufferType, const QString &target, const QString &text, const QString &sender = "", Message::Flags flags = Message::None)
    {
        emit displayMsg(networkId(), msgType, bufferType, target, text, sender, flags);
    }


signals:
    void recvRawServerMsg(QString);
    void displayStatusMsg(QString);
    void displayMsg(NetworkId, Message::Type, BufferInfo::Type, const QString &target, const QString &text, const QString &sender = "", Message::Flags flags = Message::None);
    void disconnected(NetworkId networkId);
    void connectionError(const QString &errorMsg);

    void quitRequested(NetworkId networkId);
    void sslErrors(const QVariant &errorData);

    void newEvent(Event *event);
    void socketInitialized(const CoreIdentity *identity, const QHostAddress &localAddress, quint16 localPort, const QHostAddress &peerAddress, quint16 peerPort);
    void socketDisconnected(const CoreIdentity *identity, const QHostAddress &localAddress, quint16 localPort, const QHostAddress &peerAddress, quint16 peerPort);

protected:
    inline virtual IrcChannel *ircChannelFactory(const QString &channelname) { return new CoreIrcChannel(channelname, this); }
    inline virtual IrcUser *ircUserFactory(const QString &hostmask) { return new CoreIrcUser(hostmask, this); }

protected slots:
    // TODO: remove cached cipher keys, when appropriate
    //virtual void removeIrcUser(IrcUser *ircuser);
    //virtual void removeIrcChannel(IrcChannel *ircChannel);
    //virtual void removeChansAndUsers();

private slots:
    void socketHasData();
    void socketError(QAbstractSocket::SocketError);
    void socketInitialized();
    inline void socketCloseTimeout() { socket.abort(); }
    void socketDisconnected();
    void socketStateChanged(QAbstractSocket::SocketState);
    void networkInitialized();

    void sendPerform();
    void restoreUserModes();
    void doAutoReconnect();
    void sendPing();
    void enablePingTimeout(bool enable = true);
    void disablePingTimeout();
    void sendAutoWho();
    void startAutoWhoCycle();

#ifdef HAVE_SSL
    void sslErrors(const QList<QSslError> &errors);
#endif

    void fillBucketAndProcessQueue();

    void writeToSocket(const QByteArray &data);

private:
    CoreSession *_coreSession;

#ifdef HAVE_SSL
    QSslSocket socket;
#else
    QTcpSocket socket;
#endif

    CoreUserInputHandler *_userInputHandler;

    QHash<QString, QString> _channelKeys; // stores persistent channels and their passwords, if any

    QTimer _autoReconnectTimer;
    int _autoReconnectCount;

    QTimer _socketCloseTimer;

    /* this flag triggers quitRequested() once the socket is closed
     * it is needed to determine whether or not the connection needs to be
     * in the automatic session restore. */
    bool _quitRequested;
    QString _quitReason;

    bool _previousConnectionAttemptFailed;
    int _lastUsedServerIndex;

    QTimer _pingTimer;
    uint _lastPingTime;
    uint _pingCount;
    bool _sendPings;

    QStringList _autoWhoQueue;
    QHash<QString, int> _autoWhoPending;
    QTimer _autoWhoTimer, _autoWhoCycleTimer;

    // Maintain a list of CAPs that are being checked; if empty, negotiation finished
    // See http://ircv3.net/specs/core/capability-negotiation-3.2.html
    QStringList _capsQueued;           /// Capabilities to be checked
    bool _capNegotiationActive;        /// Whether or not full capability negotiation was started
    // Avoid displaying repeat "negotiation finished" messages
    bool _capInitialNegotiationEnded;  /// Whether or not initial capability negotiation finished
    // Avoid sending repeat "CAP END" replies when registration is already ended

    /**
     * Gets the next capability to request, removing it from the queue.
     *
     * @returns Name of capability to request
     */
    QString takeQueuedCap();

    QTimer _tokenBucketTimer;
    int _messageDelay;      // token refill speed in ms
    int _burstSize;         // size of the token bucket
    int _tokenBucket;       // the virtual bucket that holds the tokens
    QList<QByteArray> _msgQueue;

    QString _requestedUserModes; // 2 strings separated by a '-' character. first part are requested modes to add, the second to remove

    // List of blowfish keys for channels
    QHash<QString, QByteArray> _cipherKeys;
};


#endif //CORENETWORK_H
