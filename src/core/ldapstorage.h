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

#ifndef LDAPSTORAGE_H
#define LDAPSTORAGE_H

#include "storage.h"

#include <ldap.h>

class LdapStorage : public Storage
{
    Q_OBJECT
public:
    LdapStorage(Storage *s, QObject *parent = 0);
    virtual ~LdapStorage();

public slots:
    virtual bool isAvailable() const;
    virtual QString displayName() const;
    virtual QString description() const;
    virtual QStringList setupKeys() const;
    virtual QVariantMap setupDefaults() const;
    virtual bool setup(const QVariantMap &settings = QVariantMap());
    virtual State init(const QVariantMap &settings = QVariantMap());
    virtual void sync();
    virtual UserId addUser(const QString &user, const QString &password);
    virtual bool updateUser(UserId user, const QString &password);
    virtual void renameUser(UserId user, const QString &newName);
    virtual UserId validateUser(const QString &user, const QString &password);
    virtual UserId getUserId(const QString &username);
    virtual UserId internalUser();
    virtual void delUser(UserId user);
    virtual void setUserSetting(UserId userId, const QString &settingName, const QVariant &data);
    virtual QVariant getUserSetting(UserId userId, const QString &settingName, const QVariant &data = QVariant());
    virtual IdentityId createIdentity(UserId user, CoreIdentity &identity);
    virtual bool updateIdentity(UserId user, const CoreIdentity &identity);
    virtual void removeIdentity(UserId user, IdentityId identityId);
    virtual QList<CoreIdentity> identities(UserId user);
    virtual NetworkId createNetwork(UserId user, const NetworkInfo &info);
    virtual bool updateNetwork(UserId user, const NetworkInfo &info);
    virtual bool removeNetwork(UserId user, const NetworkId &networkId);
    virtual QList<NetworkInfo> networks(UserId user);
    virtual QList<NetworkId> connectedNetworks(UserId user);
    virtual void setNetworkConnected(UserId user, const NetworkId &networkId, bool isConnected);
    virtual QHash<QString, QString> persistentChannels(UserId user, const NetworkId &networkId);
    virtual void setChannelPersistent(UserId user, const NetworkId &networkId, const QString &channel, bool isJoined);
    virtual void setPersistentChannelKey(UserId user, const NetworkId &networkId, const QString &channel, const QString &key);
    virtual QString awayMessage(UserId user, NetworkId networkId);
    virtual void setAwayMessage(UserId user, NetworkId networkId, const QString &awayMsg);
    virtual QString userModes(UserId user, NetworkId networkId);
    virtual void setUserModes(UserId user, NetworkId networkId, const QString &userModes);
    virtual BufferInfo bufferInfo(UserId user, const NetworkId &networkId, BufferInfo::Type type, const QString &buffer = "", bool create = true);
    virtual BufferInfo getBufferInfo(UserId user, const BufferId &bufferId);
    virtual QList<BufferInfo> requestBuffers(UserId user);
    virtual QList<BufferId> requestBufferIdsForNetwork(UserId user, NetworkId networkId);
    virtual bool removeBuffer(const UserId &user, const BufferId &bufferId);
    virtual bool renameBuffer(const UserId &user, const BufferId &bufferId, const QString &newName);
    virtual bool mergeBuffersPermanently(const UserId &user, const BufferId &bufferId1, const BufferId &bufferId2);
    virtual void setBufferLastSeenMsg(UserId user, const BufferId &bufferId, const MsgId &msgId);
    virtual QHash<BufferId, MsgId> bufferLastSeenMsgIds(UserId user);
    virtual void setBufferMarkerLineMsg(UserId user, const BufferId &bufferId, const MsgId &msgId);
    virtual QHash<BufferId, MsgId> bufferMarkerLineMsgIds(UserId user);
    virtual bool logMessage(Message &msg);
    virtual bool logMessages(MessageList &msgs);
    virtual QList<Message> requestMsgs(UserId user, BufferId bufferId, MsgId first = -1, MsgId last = -1, int limit = -1);
    virtual QList<Message> requestAllMsgs(UserId user, MsgId first = -1, MsgId last = -1, int limit = -1);

private:
    void setLdapProperties(const QVariantMap &properties);
    bool ldapConnect();
    void ldapDisconnect();
    bool ldapAuth(const QString &username, const QString &password);

private:
    Storage *m_storage;
    QByteArray m_ldapServer;
    QByteArray m_bindDN;
    QByteArray m_bindPasword;
    QByteArray m_baseDN;
    QByteArray m_ldapAttribute;
    QList<QPair<QByteArray, QByteArray> > m_requiredAttributes;
    LDAP *m_ldapConnection;
};


#endif // LDAPSTORAGE_H
