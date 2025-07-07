#ifndef IRCUSER_H
#define IRCUSER_H

#include <optional>

#include <QDateTime>
#include <QHash>
#include <QObject>
#include <QStringList>

#include "syncableobject.h"
#include "types.h"

class IrcChannel;
class Network;

class COMMON_EXPORT IrcUser : public SyncableObject
{
    Q_OBJECT
    SYNCABLE_OBJECT

public:
    IrcUser(const QString& hostmask, Network* network);

    QString hostmask() const;

    inline QString nick() const { return _nick; }
    inline QString user() const { return _user; }
    inline QString host() const { return _host; }
    inline QString realName() const { return _realName; }
    inline QString awayMessage() const { return _awayMessage; }
    inline bool away() const { return _away; }
    inline QDateTime idleTime();
    inline QDateTime loginTime() const { return _loginTime; }
    inline QString server() const { return _server; }
    inline QString ircOperator() const { return _ircOperator; }
    inline QDateTime lastAwayMessageTime() const { return _lastAwayMessageTime; }
    inline QString whoisServiceReply() const { return _whoisServiceReply; }
    inline QString suserHost() const { return _suserHost; }
    inline QString account() const { return _account; }
    inline bool encrypted() const { return _encrypted; }
    inline QString userModes() const { return _userModes; }
    QStringList channels() const;
    inline QDateTime idleTimeSet() const { return _idleTimeSet; }
    inline Network* network() const { return _network; }

    inline std::optional<QStringConverter::Encoding> codecForEncoding() const { return _codecForEncoding; }
    inline std::optional<QStringConverter::Encoding> codecForDecoding() const { return _codecForDecoding; }
    void setCodecForEncoding(const QString& name);
    void setCodecForDecoding(const QString& name);

    QString decodeString(const QByteArray& text) const;
    QByteArray encodeString(const QString& string) const;

public slots:
    void setUser(const QString& user);
    void setHost(const QString& host);
    void setNick(const QString& nick);
    void setRealName(const QString& realName);
    void setAccount(const QString& account);
    void setAway(bool away);
    void setAwayMessage(const QString& awayMessage);
    void setIdleTime(const QDateTime& idleTime);
    void setLoginTime(const QDateTime& loginTime);
    void setServer(const QString& server);
    void setIrcOperator(const QString& ircOperator);
    void setLastAwayMessage(int lastAwayMessage);
    void setLastAwayMessageTime(const QDateTime& lastAwayMessageTime);
    void setWhoisServiceReply(const QString& whoisServiceReply);
    void setSuserHost(const QString& suserHost);
    void setEncrypted(bool encrypted);
    void setUserModes(const QString& modes);
    void addUserModes(const QString& modes);
    void removeUserModes(const QString& modes);
    void updateHostmask(const QString& mask);
    void joinChannel(IrcChannel* channel, bool skip_channel_join = false);
    void joinChannel(const QString& channelname);
    void partChannel(IrcChannel* channel);
    void partChannel(const QString& channelname);
    void quit();

    void setLastChannelActivity(BufferId buffer, const QDateTime& time);
    void setLastSpokenTo(BufferId buffer, const QDateTime& time);

signals:
    void nickSet(const QString& nick);
    void userModesSet(const QString& modes);
    void userModesAdded(const QString& modes);
    void userModesRemoved(const QString& modes);
    void awaySet(bool away);
    void encryptedSet(bool encrypted);
    void quited();
    void lastChannelActivityUpdated(BufferId buffer, const QDateTime& time);
    void lastSpokenToUpdated(BufferId buffer, const QDateTime& time);

protected:
    void markAwayChanged() {}

private slots:
    void channelDestroyed();

private:
    void updateObjectName();
    void quitInternal(bool skip_sync = false);
    void partChannelInternal(IrcChannel* channel, bool skip_sync);

    bool _initialized;
    QString _nick;
    QString _user;
    QString _host;
    QString _realName;
    QString _awayMessage;
    bool _away;
    QDateTime _idleTime;
    QDateTime _idleTimeSet;
    QDateTime _loginTime;
    QString _server;
    QString _ircOperator;
    QDateTime _lastAwayMessageTime;
    QString _whoisServiceReply;
    QString _suserHost;
    QString _account;
    bool _encrypted;
    QString _userModes;

    Network* _network;
    std::optional<QStringConverter::Encoding> _codecForEncoding;
    std::optional<QStringConverter::Encoding> _codecForDecoding;
    QSet<IrcChannel*> _channels;
    QHash<BufferId, QDateTime> _lastActivity;
    QHash<BufferId, QDateTime> _lastSpokenTo;
};

#endif  // IRCUSER_H
