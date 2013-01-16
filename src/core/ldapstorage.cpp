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

#include "ldapstorage.h"

#include <logger.h>

static const QString LdapSettingServer = QString::fromLatin1("LDAP Server");
static const QString LdapSettingBindDN = QString::fromLatin1("LDAP bind DN");
static const QString LdapSettingBindPassword = QString::fromLatin1("LDAP bind password");
static const QString LdapSettingBaseDN = QString::fromLatin1("LDAP base DN");
static const QString LdapSettingUIDAttribute = QString::fromLatin1("LDAP UID attribute");
static const QString LdapSettingRequiredAttributes = QString::fromLatin1("LDAP required attributes");

LdapStorage::LdapStorage(Storage *s, QObject *parent)
    : Storage(parent)
    , m_storage(s)
    , m_ldapConnection(0)
{
}

LdapStorage::~LdapStorage()
{
    if (m_ldapConnection != 0) {
        ldap_unbind_ext(m_ldapConnection, 0, 0);
    }

    delete m_storage;
}

bool LdapStorage::isAvailable() const
{
    return m_storage->isAvailable();
}

QString LdapStorage::displayName() const
{
    return m_storage->displayName() + QString::fromLatin1("LDAP");
}

QString LdapStorage::description() const
{
    return m_storage->description() + QString::fromLatin1(" - LDAP authentication");
}

QStringList LdapStorage::setupKeys() const
{
    static const QStringList ldapSetupKeys = QStringList()
                                          << LdapSettingServer
                                          << LdapSettingBindDN
                                          << LdapSettingBindPassword
                                          << LdapSettingBaseDN
                                          << LdapSettingUIDAttribute
                                          << LdapSettingRequiredAttributes;
    return m_storage->setupKeys() + ldapSetupKeys;
}

QVariantMap LdapStorage::setupDefaults() const
{
    static QVariantMap ldapDefaults;

    if (ldapDefaults.empty()) {
        ldapDefaults.insert(LdapSettingServer, QString::fromLatin1("ldap://hostname:port"));
        ldapDefaults.insert(LdapSettingBindDN, QString());
        ldapDefaults.insert(LdapSettingBindPassword, QString());
        ldapDefaults.insert(LdapSettingBaseDN, QString::fromLatin1("ou=People,o=MyOrganization"));
        ldapDefaults.insert(LdapSettingUIDAttribute, QString::fromLatin1("uid"));
        ldapDefaults.insert(LdapSettingRequiredAttributes, QString::fromLatin1("attri1=val1;attr2=val2"));
    }

    QVariantMap defaults = m_storage->setupDefaults();
    defaults.unite(ldapDefaults);

    return defaults;
}

bool LdapStorage::setup(const QVariantMap &settings)
{
    setLdapProperties(settings);
    return m_storage->setup(settings);
}

Storage::State LdapStorage::init(const QVariantMap &settings)
{
    setLdapProperties(settings);

    quInfo() << displayName() << "storage is ready. Settings:";
    quInfo() << "Server:" << m_ldapServer;
    quInfo() << "Bind DN:" << m_bindDN;
    quInfo() << "Base DN:" << m_baseDN;
    quInfo() << "UID attribute:" << m_ldapAttribute;

    for (int i = 0; i < m_requiredAttributes.size(); ++i)
        quInfo() << "Required attributes:" << m_requiredAttributes.at(i).first << "=" << m_requiredAttributes.at(i).second;

    return m_storage->init(settings);
}

void LdapStorage::sync()
{
    m_storage->sync();
}

UserId LdapStorage::addUser(const QString &user, const QString&)
{
    return m_storage->addUser(user, QString());
}

bool LdapStorage::updateUser(UserId user, const QString&)
{
    return m_storage->updateUser(user, QString());
}

void LdapStorage::renameUser(UserId user, const QString &newName)
{
    m_storage->renameUser(user, newName);
}

UserId LdapStorage::validateUser(const QString &username, const QString &password)
{
    UserId id = m_storage->validateUser(username, QString());

    if (not ldapAuth(username, password)) {
        return UserId();
    }

    if (not id.isValid()) {
        addUser(username, QString());
    }

    return m_storage->validateUser(username, QString());
}

UserId LdapStorage::getUserId(const QString &username)
{
    return m_storage->getUserId(username);
}

UserId LdapStorage::internalUser()
{
    return m_storage->internalUser();
}

void LdapStorage::delUser(UserId user)
{
    m_storage->delUser(user);
}

void LdapStorage::setUserSetting(UserId user, const QString &settingName, const QVariant &data)
{
    m_storage->setUserSetting(user, settingName, data);
}

QVariant LdapStorage::getUserSetting(UserId user, const QString &settingName, const QVariant &data)
{
    return m_storage->getUserSetting(user, settingName, data);
}

IdentityId LdapStorage::createIdentity(UserId user, CoreIdentity &identity)
{
    return m_storage->createIdentity(user, identity);
}

bool LdapStorage::updateIdentity(UserId user, const CoreIdentity &identity)
{
    return m_storage->updateIdentity(user, identity);
}

void LdapStorage::removeIdentity(UserId user, IdentityId identityId)
{
    m_storage->removeIdentity(user, identityId);
}

QList<CoreIdentity> LdapStorage::identities(UserId user)
{
    return m_storage->identities(user);
}

NetworkId LdapStorage::createNetwork(UserId user, const NetworkInfo &info)
{
    return m_storage->createNetwork(user, info);
}

bool LdapStorage::updateNetwork(UserId user, const NetworkInfo &info)
{
    return m_storage->updateNetwork(user, info);
}

bool LdapStorage::removeNetwork(UserId user, const NetworkId &id)
{
    return m_storage->removeNetwork(user, id);
}

QList<NetworkInfo> LdapStorage::networks(UserId user)
{
    return m_storage->networks(user);
}

QList<NetworkId> LdapStorage::connectedNetworks(UserId user)
{
    return m_storage->connectedNetworks(user);
}

void LdapStorage::setNetworkConnected(UserId user, const NetworkId &networkId, bool isConnected)
{
    return m_storage->setNetworkConnected(user, networkId, isConnected);
}

QHash<QString, QString> LdapStorage::persistentChannels(UserId user, const NetworkId &networkId)
{
    return m_storage->persistentChannels(user, networkId);
}

void LdapStorage::setChannelPersistent(UserId user, const NetworkId &networkId, const QString &channel, bool isJoined)
{
    m_storage->setChannelPersistent(user, networkId, channel, isJoined);
}

void LdapStorage::setPersistentChannelKey(UserId user, const NetworkId &networkId, const QString &channel, const QString &key)
{
    m_storage->setPersistentChannelKey(user, networkId, channel, key);
}

QString LdapStorage::awayMessage(UserId user, NetworkId networkId)
{
    return m_storage->awayMessage(user, networkId);
}

void LdapStorage::setAwayMessage(UserId user, NetworkId networkId, const QString &awayMsg)
{
    m_storage->setAwayMessage(user, networkId, awayMsg);
}

QString LdapStorage::userModes(UserId user, NetworkId networkId)
{
    return m_storage->userModes(user, networkId);
}

void LdapStorage::setUserModes(UserId user, NetworkId networkId, const QString &userModes)
{
    m_storage->setUserModes(user, networkId, userModes);
}

BufferInfo LdapStorage::bufferInfo(UserId user, const NetworkId &networkId, BufferInfo::Type type, const QString &buffer, bool create)
{
    return m_storage->bufferInfo(user, networkId, type, buffer, create);
}

BufferInfo LdapStorage::getBufferInfo(UserId user, const BufferId &bufferId)
{
    return m_storage->getBufferInfo(user, bufferId);
}

QList<BufferInfo> LdapStorage::requestBuffers(UserId user)
{
    return m_storage->requestBuffers(user);
}

QList<BufferId> LdapStorage::requestBufferIdsForNetwork(UserId user, NetworkId networkId)
{
    return m_storage->requestBufferIdsForNetwork(user, networkId);
}

bool LdapStorage::removeBuffer(const UserId &user, const BufferId &bufferId)
{
    return m_storage->removeBuffer(user, bufferId);
}

bool LdapStorage::renameBuffer(const UserId &user, const BufferId &buffer, const QString &newName)
{
    return m_storage->renameBuffer(user, buffer, newName);
}

bool LdapStorage::mergeBuffersPermanently(const UserId &user, const BufferId &bufferId1, const BufferId &bufferId2)
{
    return m_storage->mergeBuffersPermanently(user, bufferId1, bufferId2);
}

void LdapStorage::setBufferLastSeenMsg(UserId user, const BufferId &bufferId, const MsgId &msgId)
{
    m_storage->setBufferLastSeenMsg(user, bufferId, msgId);
}

QHash<BufferId, MsgId> LdapStorage::bufferLastSeenMsgIds(UserId user)
{
    return m_storage->bufferLastSeenMsgIds(user);
}

void LdapStorage::setBufferMarkerLineMsg(UserId user, const BufferId &bufferId, const MsgId &msgId)
{
    m_storage->setBufferMarkerLineMsg(user, bufferId, msgId);
}

QHash<BufferId, MsgId> LdapStorage::bufferMarkerLineMsgIds(UserId user)
{
    return m_storage->bufferMarkerLineMsgIds(user);
}

bool LdapStorage::logMessage(Message &msg)
{
    return m_storage->logMessage(msg);
}

bool LdapStorage::logMessages(MessageList &msgs)
{
    return m_storage->logMessages(msgs);
}

QList<Message> LdapStorage::requestMsgs(UserId user, BufferId bufferId, MsgId first, MsgId last, int limit)
{
    return m_storage->requestMsgs(user, bufferId, first, last, limit);
}

QList<Message> LdapStorage::requestAllMsgs(UserId user, MsgId first, MsgId last, int limit)
{
    return m_storage->requestAllMsgs(user, first, last, limit);
}

///////////////////////////////////////////////////////////////////////////////

void LdapStorage::setLdapProperties(const QVariantMap &properties)
{
    m_ldapServer = properties.value(LdapSettingServer).toByteArray();
    m_bindDN = properties.value(LdapSettingBindDN).toByteArray();
    m_bindPasword = properties.value(LdapSettingBindPassword).toByteArray();
    m_baseDN = properties.value(LdapSettingBaseDN).toByteArray();
    m_ldapAttribute = properties.value(LdapSettingUIDAttribute).toByteArray();

    foreach (const QByteArray &str, properties.value(LdapSettingRequiredAttributes).toByteArray().split(';')) {
        const QByteArray trimmed = str.trimmed();

        if (trimmed.isEmpty()) {
            continue;
        }

        const QList<QByteArray> tokens = trimmed.split('=');

        if (tokens.size() != 2) {
            continue;
        }

        m_requiredAttributes.append(qMakePair(tokens.at(0).trimmed(), tokens.at(1).trimmed()));
    }
}

bool LdapStorage::ldapConnect()
{
    if (m_ldapConnection != 0) {
        ldapDisconnect();
    }

    int res, v = LDAP_VERSION3;

    res = ldap_initialize(&m_ldapConnection, m_ldapServer.constData());

    if (res != LDAP_SUCCESS) {
        qWarning() << "Could not connect to LDAP server:" << ldap_err2string(res);
        return false;
    }

    res = ldap_set_option(m_ldapConnection, LDAP_OPT_PROTOCOL_VERSION, (void*)&v);

    if (res != LDAP_SUCCESS) {
        qWarning() << "Could not set LDAP protocol version to v3:" << ldap_err2string(res);
        ldap_unbind_ext(m_ldapConnection, 0, 0);
        m_ldapConnection = 0;
        return false;
    }

    return true;
}

void LdapStorage::ldapDisconnect()
{
    if (m_ldapConnection == 0) {
        return;
    }

    ldap_unbind_ext(m_ldapConnection, 0, 0);
    m_ldapConnection = 0;
}

bool LdapStorage::ldapAuth(const QString &username, const QString &password)
{
    if (password.isEmpty()) {
        return false;
    }

    int res;

    if (m_ldapConnection == 0) {
        if (not ldapConnect()) {
            return false;
        }
    }

    struct berval cred;

    cred.bv_val = const_cast<char*>(m_bindPasword.size() > 0 ? m_bindPasword.constData() : NULL);
    cred.bv_len = m_bindPasword.size();

    res = ldap_sasl_bind_s(m_ldapConnection,
                           m_bindDN.size() > 0 ? m_bindDN.constData() : 0,
                           LDAP_SASL_SIMPLE,
                           &cred,
                           0,
                           0,
                           0);

    if (res != LDAP_SUCCESS) {
        qWarning() << "Refusing connection from" << username << "(LDAP bind failed:" << ldap_err2string(res) << ")";
        ldapDisconnect();
        return false;
    }

    LDAPMessage *msg = NULL, *entry = NULL;

    const QByteArray ldapQuery = m_ldapAttribute + '=' + username.toLocal8Bit();

    res = ldap_search_ext_s(m_ldapConnection,
                            m_baseDN.constData(),
                            LDAP_SCOPE_SUBTREE,
                            ldapQuery.constData(),
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            &msg);

    if (res != LDAP_SUCCESS) {
        qWarning() << "Refusing connection from" << username << "(LDAP search failed:" << ldap_err2string(res) << ")";
        return false;
    }

    if (ldap_count_entries(m_ldapConnection, msg) > 1) {
        qWarning() << "Refusing connection from" << username << "(LDAP search returned more than one result)";
        ldap_msgfree(msg);
        return false;
    }

    entry = ldap_first_entry(m_ldapConnection, msg);

    if (entry == 0) {
        qWarning() << "Refusing connection from" << username << "(LDAP search returned no results)";
        ldap_msgfree(msg);
        return false;
    }

    const QByteArray password_c = password.toLocal8Bit();

    cred.bv_val = const_cast<char*>(password_c.constData());
    cred.bv_len = password.size();

    char *userDN = ldap_get_dn(m_ldapConnection, entry);

    res = ldap_sasl_bind_s(m_ldapConnection,
                           userDN,
                           LDAP_SASL_SIMPLE,
                           &cred,
                           0,
                           0,
                           0);

    if (res != LDAP_SUCCESS) {
        qWarning() << "Refusing connection from" << username << "(LDAP authentication failed)";
        ldap_memfree(userDN);
        ldap_msgfree(msg);
        return false;
    }

    if (m_requiredAttributes.isEmpty()) {
        ldap_memfree(userDN);
        ldap_msgfree(msg);
        return true;
    }

    bool authenticated = false;

    // Needed to use foreach
    typedef QPair<QByteArray, QByteArray> QByteArrayPair;

    foreach (const QByteArrayPair &pair, m_requiredAttributes) {
        const QByteArray &attribute = pair.first;
        const QByteArray &value = pair.second;

        struct berval attr_value;
        attr_value.bv_val = const_cast<char*>(value.constData());
        attr_value.bv_len = value.size();

        authenticated = (ldap_compare_ext_s(m_ldapConnection,
                                            userDN,
                                            attribute.constData(),
                                            &attr_value,
                                            0,
                                            0) == LDAP_COMPARE_TRUE);

        if (authenticated) {
            break;
        }
    }

    ldap_memfree(userDN);
    ldap_msgfree(msg);

    return authenticated;
}
