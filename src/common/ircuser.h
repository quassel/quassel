/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel Project                          *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _IRCUSER_H_
#define _IRCUSER_H_

#include <QSet>
#include <QString>
#include <QStringList>
#include <QVariantMap>

class SignalProxy;
class NetworkInfo;
class IrcChannel;

class IrcUser : public QObject {
  Q_OBJECT

  Q_PROPERTY(QString user READ user WRITE setUser STORED false)
  Q_PROPERTY(QString host READ host WRITE setHost STORED false)
  Q_PROPERTY(QString nick READ nick WRITE setNick STORED false)

  Q_PROPERTY(QStringList channels READ channels STORED false)
  //  Q_PROPERTY(QStringList usermodes READ usermodes WRITE setUsermodes)

public:
  IrcUser(const QString &hostmask, NetworkInfo *networkInfo);
  virtual ~IrcUser();

  bool initialized() const;

  QString user() const;
  QString host() const;
  QString nick() const;
  QString hostmask() const;

  QString userModes() const;

  QStringList channels() const;

  // user-specific encodings
  QTextCodec *codecForEncoding() const;
  QTextCodec *codecForDecoding() const;
  void setCodecForEncoding(const QString &codecName);
  void setCodecForEncoding(QTextCodec *codec);
  void setCodecForDecoding(const QString &codecName);
  void setCodecForDecoding(QTextCodec *codec);

  QString decodeString(const QByteArray &text) const;
  QByteArray encodeString(const QString string) const;

public slots:
  void setUser(const QString &user);
  void setHost(const QString &host);
  void setNick(const QString &nick);
  void updateHostmask(const QString &mask);

  void setUserModes(const QString &modes);

  void joinChannel(IrcChannel *channel);
  void joinChannel(const QString &channelname);
  void partChannel(IrcChannel *channel);
  void partChannel(const QString &channelname);

  void addUserMode(const QString &mode);
  void removeUserMode(const QString &mode);

  // init seters
  void initSetChannels(const QStringList channels);

  void setInitialized();

signals:
  void userSet(QString user);
  void hostSet(QString host);
  void nickSet(QString newnick);
  void hostmaskUpdated(QString mask);

  void userModesSet(QString modes);

  void channelJoined(QString channel);
  void channelParted(QString channel);

  void userModeAdded(QString mode);
  void userModeRemoved(QString mode);

  void renameObject(QString oldname, QString newname);

//   void setUsermodes(const QSet<QString> &usermodes);
//   QSet<QString> usermodes() const;

  void initDone();

private slots:
  void updateObjectName();
  void channelDestroyed();

private:
  inline bool operator==(const IrcUser &ircuser2) {
    return (_nick.toLower() == ircuser2.nick().toLower());
  }

  inline bool operator==(const QString &nickname) {
    return (_nick.toLower() == nickname.toLower());
  }

  bool _initialized;

  QString _nick;
  QString _user;
  QString _host;

  // QSet<QString> _channels;
  QSet<IrcChannel *> _channels;
  QString _userModes;

  NetworkInfo *networkInfo;

  QTextCodec *_codecForEncoding;
  QTextCodec *_codecForDecoding;
};

#endif
