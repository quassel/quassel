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

    QVariantMap toVariantMap() override;
    void fromVariantMap(const QVariantMap& map) override;

public slots:
    void setTopic(const QString& topic);
    void setPassword(const QString& password);
    void setEncrypted(bool encrypted);
    void joinIrcUsers(const QStringList& nicks, const QStringList& modes);
    void joinIrcUser(IrcUser* user);
    void part(IrcUser* user);
    void partChannel();
    void setUserModes(IrcUser* user, const QString& modes);
    void addUserMode(IrcUser* user, const QString& mode);
    void removeUserMode(IrcUser* user, const QString& mode);
    void addChannelMode(const QString& mode, const QString& param = QString());
    void removeChannelMode(const QString& mode, const QString& param = QString());

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

    friend class Network;
};

#endif  // IRCCHANNEL_H
