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

#ifndef CORESESSION_H
#define CORESESSION_H

#include <QString>
#include <QVariant>

#include "corecoreinfo.h"
#include "corealiasmanager.h"
#include "corehighlightrulemanager.h"
#include "coreignorelistmanager.h"
#include "peer.h"
#include "protocol.h"
#include "message.h"
#include "storage.h"

class CoreBacklogManager;
class CoreBufferSyncer;
class CoreBufferViewManager;
class CoreDccConfig;
class CoreIdentity;
class CoreIrcListHelper;
class CoreNetwork;
class CoreNetworkConfig;
class CoreSessionEventProcessor;
class CoreTransferManager;
class CtcpParser;
class EventManager;
class EventStringifier;
class InternalPeer;
class IrcParser;
class MessageEvent;
class NetworkConnection;
class RemotePeer;
class SignalProxy;

struct NetworkInfo;

class QScriptEngine;

class CoreSession : public QObject
{
    Q_OBJECT

public:
    CoreSession(UserId, bool restoreState, QObject *parent = 0);
    ~CoreSession();

    QList<BufferInfo> buffers() const;
    inline UserId user() const { return _user; }
    CoreNetwork *network(NetworkId) const;
    CoreIdentity *identity(IdentityId) const;
    const QString strictSysident();
    inline CoreNetworkConfig *networkConfig() const { return _networkConfig; }
    NetworkConnection *networkConnection(NetworkId) const;

    Protocol::SessionState sessionState() const;

    inline SignalProxy *signalProxy() const { return _signalProxy; }

    const AliasManager &aliasManager() const { return _aliasManager; }
    AliasManager &aliasManager() { return _aliasManager; }

    inline EventManager *eventManager() const { return _eventManager; }
    inline EventStringifier *eventStringifier() const { return _eventStringifier; }
    inline CoreSessionEventProcessor *sessionEventProcessor() const { return _sessionEventProcessor; }
    inline CtcpParser *ctcpParser() const { return _ctcpParser; }
    inline IrcParser *ircParser() const { return _ircParser; }

    inline CoreIrcListHelper *ircListHelper() const { return _ircListHelper; }

    inline CoreIgnoreListManager *ignoreListManager() { return &_ignoreListManager; }
    inline HighlightRuleManager *highlightRuleManager() { return &_highlightRuleManager; }
    inline CoreTransferManager *transferManager() const { return _transferManager; }
    inline CoreDccConfig *dccConfig() const { return _dccConfig; }

//   void attachNetworkConnection(NetworkConnection *conn);

    //! Return necessary data for restoring the session after restarting the core
    void restoreSessionState();

public slots:
    void addClient(RemotePeer *peer);
    void addClient(InternalPeer *peer);

    void msgFromClient(BufferInfo, QString message);

    //! Create an identity and propagate the changes to the clients.
    /** \param identity The identity to be created.
     */
    void createIdentity(const Identity &identity, const QVariantMap &additional);
    void createIdentity(const CoreIdentity &identity);

    //! Remove identity and propagate that fact to the clients.
    /** \param identity The identity to be removed.
     */
    void removeIdentity(IdentityId identity);

    //! Create a network and propagate the changes to the clients.
    /** \param info The network's settings.
     */
    void createNetwork(const NetworkInfo &info, const QStringList &persistentChannels = QStringList());

    //! Remove network and propagate that fact to the clients.
    /** \param network The id of the network to be removed.
     */
    void removeNetwork(NetworkId network);

    //! Rename a Buffer for a given network
    /* \param networkId The id of the network the buffer belongs to
     * \param newName   The new name of the buffer
     * \param oldName   The old name of the buffer
     */
    void renameBuffer(const NetworkId &networkId, const QString &newName, const QString &oldName);

    void changePassword(PeerPtr peer, const QString &userName, const QString &oldPassword, const QString &newPassword);

    void kickClient(int peerId);

    QHash<QString, QString> persistentChannels(NetworkId) const;

    QHash<QString, QByteArray> bufferCiphers(NetworkId id) const;
    void setBufferCipher(NetworkId id, const QString &bufferName, const QByteArray &cipher) const;

    /**
     * Marks us away (or unaway) on all networks
     *
     * @param[in] msg             Away message, or blank to set unaway
     * @param[in] skipFormatting  If true, skip timestamp formatting codes (e.g. if already done)
     */
    void globalAway(const QString &msg = QString(), const bool skipFormatting = false);

signals:
    void initialized();
    void sessionState(const Protocol::SessionState &sessionState);

    //void msgFromGui(uint netid, QString buf, QString message);
    void displayMsg(Message message);
    void displayStatusMsg(QString, QString);

    void scriptResult(QString result);

    //! Identity has been created.
    /** This signal is propagated to the clients to tell them that the given identity has been created.
     *  \param identity The new identity.
     */
    void identityCreated(const Identity &identity);

    //! Identity has been removed.
    /** This signal is propagated to the clients to inform them about the removal of the given identity.
     *  \param identity The identity that has been removed.
     */
    void identityRemoved(IdentityId identity);

    void networkCreated(NetworkId);
    void networkRemoved(NetworkId);
    void networkDisconnected(NetworkId);

    void passwordChanged(PeerPtr peer, bool success);

    void disconnectFromCore();

protected:
    virtual void customEvent(QEvent *event);

private slots:
    void removeClient(Peer *peer);

    void recvStatusMsgFromServer(QString msg);
    void recvMessageFromServer(NetworkId networkId, Message::Type, BufferInfo::Type, const QString &target, const QString &text, const QString &sender = "", Message::Flags flags = Message::None);

    void destroyNetwork(NetworkId);

    void scriptRequest(QString script);

    void clientsConnected();
    void clientsDisconnected();

    void updateIdentityBySender();

    void saveSessionState() const;

private:
    void processMessages();

    void loadSettings();
    void initScriptEngine();

    /// Hook for converting events to the old displayMsg() handlers
    Q_INVOKABLE void processMessageEvent(MessageEvent *event);

    UserId _user;

    SignalProxy *_signalProxy;
    CoreAliasManager _aliasManager;
    // QHash<NetworkId, NetworkConnection *> _connections;
    QHash<NetworkId, CoreNetwork *> _networks;
    //  QHash<NetworkId, CoreNetwork *> _networksToRemove;
    QHash<IdentityId, CoreIdentity *> _identities;

    CoreBufferSyncer *_bufferSyncer;
    CoreBacklogManager *_backlogManager;
    CoreBufferViewManager *_bufferViewManager;
    CoreDccConfig *_dccConfig;
    CoreIrcListHelper *_ircListHelper;
    CoreNetworkConfig *_networkConfig;
    CoreCoreInfo _coreInfo;
    CoreTransferManager *_transferManager;

    EventManager *_eventManager;
    EventStringifier *_eventStringifier; // should eventually move into client
    CoreSessionEventProcessor *_sessionEventProcessor;
    CtcpParser *_ctcpParser;
    IrcParser *_ircParser;

    QScriptEngine *scriptEngine;

    /**
     * This method obtains the prefixes of the message's sender within a channel, by looking up their channelmodes, and
     * processing them to prefixes based on the network's settings.
     * @param sender The hostmask of the sender
     * @param bufferInfo The BufferInfo object of the buffer
     */
    QString senderPrefixes(const QString &sender, const BufferInfo &bufferInfo) const;
    QList<RawMessage> _messageQueue;
    bool _processMessages;
    CoreIgnoreListManager _ignoreListManager;
    CoreHighlightRuleManager _highlightRuleManager;
};


struct RawMessage {
    NetworkId networkId;
    Message::Type type;
    BufferInfo::Type bufferType;
    QString target;
    QString text;
    QString sender;
    Message::Flags flags;
    RawMessage(NetworkId networkId, Message::Type type, BufferInfo::Type bufferType, const QString &target, const QString &text, const QString &sender, Message::Flags flags)
        : networkId(networkId), type(type), bufferType(bufferType), target(target), text(text), sender(sender), flags(flags) {}
};

#endif
