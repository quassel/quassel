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

#include <functional>

#include <QSslError>
#include <QSslSocket>
#include <QTimer>

#ifdef HAVE_QCA2
#    include "cipher.h"
#endif

#include "coreircchannel.h"
#include "coreircuser.h"
#include "coresession.h"
#include "irccap.h"
#include "irctag.h"
#include "network.h"

class CoreIdentity;
class CoreUserInputHandler;
class CoreIgnoreListManager;
class Event;

class CoreNetwork : public Network
{
    Q_OBJECT

public:
    CoreNetwork(const NetworkId& networkid, CoreSession* session);
    ~CoreNetwork() override;

    inline CoreIdentity* identityPtr() const { return coreSession()->identity(identity()); }
    inline CoreSession* coreSession() const { return _coreSession; }
    inline CoreNetworkConfig* networkConfig() const { return coreSession()->networkConfig(); }

    inline CoreUserInputHandler* userInputHandler() const { return _userInputHandler; }
    inline CoreIgnoreListManager* ignoreListManager() { return coreSession()->ignoreListManager(); }

    //! Decode a string using the server (network) decoding.
    inline QString serverDecode(const QByteArray& string) const { return decodeServerString(string); }

    //! Decode a string using a channel-specific encoding if one is set (and use the standard encoding else).
    QString channelDecode(const QString& channelName, const QByteArray& string) const;

    //! Decode a string using an IrcUser-specific encoding, if one exists (using the standaed encoding else).
    QString userDecode(const QString& userNick, const QByteArray& string) const;

    //! Encode a string using the server (network) encoding.
    inline QByteArray serverEncode(const QString& string) const { return encodeServerString(string); }

    //! Encode a string using the channel-specific encoding, if set, and use the standard encoding else.
    QByteArray channelEncode(const QString& channelName, const QString& string) const;

    //! Encode a string using the user-specific encoding, if set, and use the standard encoding else.
    QByteArray userEncode(const QString& userNick, const QString& string) const;

    inline QString channelKey(const QString& channel) const { return _channelKeys.value(channel.toLower(), QString()); }

    inline QByteArray readChannelCipherKey(const QString& channel) const { return _cipherKeys.value(channel.toLower()); }
    inline void storeChannelCipherKey(const QString& channel, const QByteArray& key) { _cipherKeys[channel.toLower()] = key; }

    /**
     * Checks if the given target has an automatic WHO in progress
     *
     * @param name Channel or nickname
     * @return True if an automatic WHO is in progress, otherwise false
     */
    inline bool isAutoWhoInProgress(const QString& name) const { return _autoWhoPending.value(name.toLower(), 0); }

    inline UserId userId() const { return _coreSession->user(); }

    inline QAbstractSocket::SocketState socketState() const { return socket.state(); }
    inline bool socketConnected() const { return socket.state() == QAbstractSocket::ConnectedState; }
    inline QHostAddress localAddress() const { return socket.localAddress(); }
    inline QHostAddress peerAddress() const { return socket.peerAddress(); }
    inline quint16 localPort() const { return socket.localPort(); }
    inline quint16 peerPort() const { return socket.peerPort(); }

    /**
     * Gets whether or not a disconnect was expected.
     *
     * Distinguishes desired quits from unexpected disconnections such as socket errors or timeouts.
     *
     * @return True if disconnect was requested, otherwise false.
     */
    inline bool disconnectExpected() const { return _disconnectExpected; }

    /**
     * Gets whether or not the server replies to automated PINGs with a valid timestamp
     *
     * Distinguishes between servers that reply by quoting the text sent, and those that respond
     * with whatever they want.
     *
     * @return True if a valid timestamp has been received as a PONG, otherwise false.
     */
    inline bool isPongTimestampValid() const { return _pongTimestampValid; }

    /**
     * Gets whether or not an automated PING has been sent without any PONG received
     *
     * Reset whenever any PONG is received, not just the automated one sent.
     *
     * @return True if a PING has been sent without a PONG received, otherwise false.
     */
    inline bool isPongReplyPending() const { return _pongReplyPending; }

    QList<QList<QByteArray>> splitMessage(const QString& cmd,
                                          const QString& message,
                                          const std::function<QList<QByteArray>(QString&)>& cmdGenerator);

    // IRCv3 capability negotiation

    /**
     * Checks if capability negotiation is currently ongoing.
     *
     * @returns True if in progress, otherwise false
     */
    inline bool capsPendingNegotiation() const { return (!_capsQueuedIndividual.empty() || !_capsQueuedBundled.empty()); }

    /**
     * Queues a capability to be requested.
     *
     * Adds to the list of capabilities being requested.  If non-empty, CAP REQ messages are sent
     * to the IRC server.  This may happen at login or if capabilities are announced via CAP NEW.
     *
     * @param[in] capability Name of the capability
     */
    void queueCap(const QString& capability);

    /**
     * Begins capability negotiation if capabilities are queued, otherwise returns.
     *
     * If any capabilities are queued, this will begin the cycle of taking each capability and
     * requesting it.  When no capabilities remain, capability negotiation is suitably ended.
     */
    void beginCapNegotiation();

    /**
     * Ends capability negotiation.
     *
     * This won't have effect if other CAP commands are in the command queue before calling this
     * command.  It should only be called when capability negotiation is complete.
     */
    void endCapNegotiation();

    /**
     * Queues the most recent capability set for retrying individually.
     *
     * Retries the most recent bundle of capabilities one at a time instead of as a group, working
     * around the issue that IRC servers can deny a group of requested capabilities without
     * indicating which capabilities failed.
     *
     * See: http://ircv3.net/specs/core/capability-negotiation-3.1.html
     *
     * This does NOT call CoreNetwork::sendNextCap().  Call that when ready afterwards.  Does
     * nothing if the last capability tried was individual instead of a set.
     */
    void retryCapsIndividually();

    /**
     * List of capabilities requiring further core<->server messages to configure.
     *
     * For example, SASL requires the back-and-forth of AUTHENTICATE, so the next capability cannot
     * be immediately sent.
     *
     * Any capabilities in this list must call CoreNetwork::sendNextCap() on their own and they will
     * not be batched together with other capabilities.
     *
     * See: http://ircv3.net/specs/extensions/sasl-3.2.html
     */
    const QStringList capsRequiringConfiguration = QStringList{IrcCap::SASL};

public slots:
    void setMyNick(const QString& mynick);
    void requestConnect() const;
    void requestDisconnect() const;
    void requestSetNetworkInfo(const NetworkInfo& info);

    void setUseAutoReconnect(bool);
    void setAutoReconnectInterval(quint32);
    void setAutoReconnectRetries(quint16);

    void setPingInterval(int interval);

    /**
     * Sets whether or not the IRC server has replied to PING with a valid timestamp
     *
     * This allows determining whether or not an IRC server responds to PING with a PONG that quotes
     * what was sent, or if it does something else (and therefore PONGs should be more aggressively
     * hidden).
     *
     * @param timestampValid If true, a valid timestamp has been received via PONG reply
     */
    void setPongTimestampValid(bool validTimestamp);

    /**
     * Indicates that the CoreSession is shutting down.
     *
     * Disconnects the network if connected, and sets a flag that prevents reconnections.
     */
    void shutdown();

    void connectToIrc(bool reconnecting = false);
    /**
     * Disconnect from the IRC server.
     *
     * Begin disconnecting from the IRC server, including optionally reconnecting.
     *
     * @param requested       If true, user requested this disconnect; don't try to reconnect
     * @param reason          Reason for quitting, defaulting to the user-configured quit reason
     * @param withReconnect   Reconnect to the network after disconnecting (e.g. ping timeout)
     */
    void disconnectFromIrc(bool requested = true, const QString& reason = QString(), bool withReconnect = false);

    /**
     * Forcibly close the IRC server socket, waiting for it to close.
     *
     * Call CoreNetwork::disconnectFromIrc() first, allow the event loop to run, then if you need to
     * be sure the network's disconnected (e.g. clean-up), call this.
     *
     * @param msecs  Maximum time to wait for socket to close, in milliseconds.
     * @return True if socket closes successfully; false if error occurs or timeout reached
     */
    bool forceDisconnect(int msecs = 1000);

    void userInput(const BufferInfo& bufferInfo, QString msg);

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
    void putRawLine(const QByteArray& input, bool prepend = false);

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
    void putCmd(const QString& cmd,
                const QList<QByteArray>& params,
                const QByteArray& prefix = {},
                const QHash<IrcTagKey, QString>& tags = {},
                bool prepend = false);

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
    void putCmd(const QString& cmd,
                const QList<QList<QByteArray>>& params,
                const QByteArray& prefix = {},
                const QHash<IrcTagKey, QString>& tags = {},
                bool prependAll = false);

    void setChannelJoined(const QString& channel);
    void setChannelParted(const QString& channel);
    void addChannelKey(const QString& channel, const QString& key);
    void removeChannelKey(const QString& channel);

    // Blowfish stuff
#ifdef HAVE_QCA2
    Cipher* cipher(const QString& recipient);
    QByteArray cipherKey(const QString& recipient) const;
    void setCipherKey(const QString& recipient, const QByteArray& key);
    bool cipherUsesCBC(const QString& target);
#endif

    // Custom rate limiting (can be connected to signals)

    /**
     * Update rate limiting according to Network configuration
     *
     * Updates the token bucket and message queue timer according to the network configuration, such
     * as on first load, or after changing settings.
     *
     * Calling this will reset any ongoing queue delays.  If messages exist in the queue when rate
     * limiting is disabled, messages will be quickly sent (100 ms) with new messages queued to send
     * until the queue is cleared.
     *
     * @see Network::useCustomMessageRate()
     * @see Network::messageRateBurstSize()
     * @see Network::messageRateDelay()
     * @see Network::unlimitedMessageRate()
     *
     * @param[in] forceUnlimited
     * @parmblock
     * If true, override user settings to disable message rate limiting, otherwise apply rate limits
     * set by the user.  Use with caution and remember to re-enable configured limits when done.
     * @endparmblock
     */
    void updateRateLimiting(bool forceUnlimited = false);

    /**
     * Resets the token bucket up to the maximum
     *
     * Call this if the connection's been reset after calling updateRateLimiting() if needed.
     *
     * @see CoreNetwork::updateRateLimiting()
     */
    void resetTokenBucket();

    // IRCv3 capability negotiation (can be connected to signals)

    /**
     * Indicates a capability is now available, with optional value in Network::capValue().
     *
     * @see Network::addCap()
     *
     * @param[in] capability Name of the capability
     */
    void serverCapAdded(const QString& capability);

    /**
     * Indicates a capability was acknowledged (enabled by the IRC server).
     *
     * @see Network::acknowledgeCap()
     *
     * @param[in] capability Name of the capability
     */
    void serverCapAcknowledged(const QString& capability);

    /**
     * Indicates a capability was removed from the list of available capabilities.
     *
     * @see Network::removeCap()
     *
     * @param[in] capability Name of the capability
     */
    void serverCapRemoved(const QString& capability);

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
     * @param[in] name Channel or nickname
     */
    void queueAutoWhoOneshot(const QString& name);

    /**
     * Checks if the given target has an automatic WHO in progress, and sets it as done if so
     *
     * @param name Channel or nickname
     * @return True if an automatic WHO is in progress (and should be silenced), otherwise false
     */
    bool setAutoWhoDone(const QString& name);

    void updateIssuedModes(const QString& requestedModes);
    void updatePersistentModes(QString addModes, QString removeModes);
    void resetPersistentModes();

    Server usedServer() const;

    inline void resetPingTimeout() { _pingCount = 0; }

    /**
     * Marks the network as no longer having a pending reply to an automated PING
     */
    inline void resetPongReplyPending() { _pongReplyPending = false; }

    void onDisplayMsg(const NetworkInternalMessage& msg) { emit displayMsg(RawMessage(networkId(), msg)); }

signals:
    void recvRawServerMsg(const QString&);
    void displayStatusMsg(const QString&);
    void displayMsg(const RawMessage& msg);
    void disconnected(NetworkId networkId);
    void connectionError(const QString& errorMsg);

    void quitRequested(NetworkId networkId);
    void sslErrors(const QVariant& errorData);

    void newEvent(Event* event);
    void socketInitialized(const CoreIdentity* identity,
                           const QHostAddress& localAddress,
                           quint16 localPort,
                           const QHostAddress& peerAddress,
                           quint16 peerPort,
                           qint64 socketId);
    void socketDisconnected(const CoreIdentity* identity,
                            const QHostAddress& localAddress,
                            quint16 localPort,
                            const QHostAddress& peerAddress,
                            quint16 peerPort,
                            qint64 socketId);

protected:
    inline IrcChannel* ircChannelFactory(const QString& channelname) { return new CoreIrcChannel(channelname, this); }
    inline IrcUser* ircUserFactory(const QString& hostmask) { return new CoreIrcUser(hostmask, this); }

protected slots:
    // TODO: remove cached cipher keys, when appropriate
    // virtual void removeIrcUser(IrcUser *ircuser);
    // virtual void removeIrcChannel(IrcChannel *ircChannel);
    // virtual void removeChansAndUsers();

private slots:
    void onSocketHasData();
    void onSocketError(QAbstractSocket::SocketError);
    void onSocketInitialized();
    void onSocketCloseTimeout();
    void onSocketDisconnected();
    void onSocketStateChanged(QAbstractSocket::SocketState);

    void networkInitialized();

    void sendPerform();
    void restoreUserModes();
    void doAutoReconnect();
    void sendPing();
    void enablePingTimeout(bool enable = true);
    void disablePingTimeout();
    void sendAutoWho();
    void startAutoWhoCycle();

    void onSslErrors(const QList<QSslError>& errors);

    /**
     * Check the message token bucket
     *
     * If rate limiting is disabled and the message queue is empty, this disables the token bucket
     * timer.  Otherwise, a queued message will be sent.
     *
     * @see CoreNetwork::fillBucketAndProcessQueue()
     */
    void checkTokenBucket();

    /**
     * Top up token bucket and send as many queued messages as possible
     *
     * If there's any room for more tokens, add to the token bucket.  Separately, if there's any
     * messages to send, send until there's no more tokens or the queue is empty, whichever comes
     * first.
     */
    void fillBucketAndProcessQueue();

    void writeToSocket(const QByteArray& data);

private:
    void showMessage(const NetworkInternalMessage& msg) { emit displayMsg(RawMessage(networkId(), msg)); }

private:
    CoreSession* _coreSession;

    bool _debugLogRawIrc;      ///< If true, include raw IRC socket messages in the debug log
    qint32 _debugLogRawNetId;  ///< Network ID for logging raw IRC socket messages, or -1 for all

    QSslSocket socket;
    qint64 _socketId{0};

    CoreUserInputHandler* _userInputHandler;
    MetricsServer* _metricsServer;

    QHash<QString, QString> _channelKeys;  // stores persistent channels and their passwords, if any

    QTimer _autoReconnectTimer;
    int _autoReconnectCount;

    QTimer _socketCloseTimer;

    /* this flag triggers quitRequested() once the socket is closed
     * it is needed to determine whether or not the connection needs to be
     * in the automatic session restore. */
    bool _quitRequested;
    QString _quitReason;

    bool _disconnectExpected;  /// If true, connection is quitting, expect a socket close
    // This avoids logging a spurious RemoteHostClosedError whenever disconnect is called without
    // specifying a permanent (saved to core session) disconnect.

    bool _shuttingDown{false};  ///< If true, we're shutting down and ignore requests to (dis)connect networks

    bool _previousConnectionAttemptFailed;
    int _lastUsedServerIndex;

    QTimer _pingTimer;
    qint64 _lastPingTime = 0;          ///< Unix time of most recently sent automatic ping
    uint _pingCount = 0;               ///< Unacknowledged automatic pings
    bool _sendPings = false;           ///< If true, pings should be periodically sent to server
    bool _pongTimestampValid = false;  ///< If true, IRC server responds to PING by quoting in PONG
    // This tracks whether or not a server responds to PING with a PONG of what was sent, or if it
    // does something else.  If false, PING reply hiding should be more aggressive.
    bool _pongReplyPending = false;  ///< If true, at least one PING sent without a PONG reply

    QStringList _autoWhoQueue;
    QHash<QString, int> _autoWhoPending;
    QTimer _autoWhoTimer, _autoWhoCycleTimer;

    // Maintain a list of CAPs that are being checked; if empty, negotiation finished
    // See http://ircv3.net/specs/core/capability-negotiation-3.2.html
    QStringList _capsQueuedIndividual;  /// Capabilities to check that require one at a time requests
    QStringList _capsQueuedBundled;     /// Capabilities to check that can be grouped together
    QStringList _capsQueuedLastBundle;  /// Most recent capability bundle requested (no individuals)
    // Some capabilities, such as SASL, require follow-up messages to be fully enabled.  These
    // capabilities should not be grouped with others to avoid requesting new capabilities while the
    // previous capability is still being set up.
    // Additionally, IRC servers can choose to send a 'NAK' to any set of requested capabilities.
    // If this happens, we need a way to retry each capability individually in order to avoid having
    // one failing capability (e.g. SASL) block all other capabilities.

    bool _capNegotiationActive;  /// Whether or not full capability negotiation was started
    // Avoid displaying repeat "negotiation finished" messages
    bool _capInitialNegotiationEnded;  /// Whether or not initial capability negotiation finished
    // Avoid sending repeat "CAP END" replies when registration is already ended

    /**
     * Gets the next set of capabilities to request, removing them from the queue.
     *
     * May return one or multiple space-separated capabilities, depending on queue.
     *
     * @returns Space-separated names of capabilities to request, or empty string if none remain
     */
    QString takeQueuedCaps();

    /**
     * Maximum length of a single 'CAP REQ' command.
     *
     * To be safe, 100 chars.  Higher numbers should be possible; this is following the conservative
     * minimum number of characters that IRC servers must return in CAP NAK replies.  This also
     * means CAP NAK replies will contain the full list of denied capabilities.
     *
     * See: http://ircv3.net/specs/core/capability-negotiation-3.1.html
     */
    const int maxCapRequestLength = 100;

    QTimer _tokenBucketTimer;
    // No need for int type as one cannot travel into the past (at least not yet, Doc)
    quint32 _messageDelay;        /// Token refill speed in ms
    quint32 _burstSize;           /// Size of the token bucket
    quint32 _tokenBucket;         /// The virtual bucket that holds the tokens
    QList<QByteArray> _msgQueue;  /// Queue of messages waiting to be sent
    bool _skipMessageRates;       /// If true, skip all message rate limits

    QString _requestedUserModes;  // 2 strings separated by a '-' character. first part are requested modes to add, the second to remove

    // List of blowfish keys for channels
    QHash<QString, QByteArray> _cipherKeys;
};
