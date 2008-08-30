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

#ifndef _IDENTITY_H_
#define _IDENTITY_H_

#include <QDataStream>
#include <QMetaType>
#include <QString>
#include <QStringList>

#include "types.h"
#include "syncableobject.h"

class Identity : public SyncableObject {
  Q_OBJECT

  Q_PROPERTY(IdentityId identityId READ id WRITE setId STORED false)
  Q_PROPERTY(QString identityName READ identityName WRITE setIdentityName STORED false)
  Q_PROPERTY(QString realName READ realName WRITE setRealName STORED false)
  Q_PROPERTY(QStringList nicks READ nicks WRITE setNicks STORED false)
  Q_PROPERTY(QString awayNick READ awayNick WRITE setAwayNick STORED false)
  Q_PROPERTY(bool awayNickEnabled READ awayNickEnabled WRITE setAwayNickEnabled STORED false)
  Q_PROPERTY(QString awayReason READ awayReason WRITE setAwayReason STORED false)
  Q_PROPERTY(bool awayReasonEnabled READ awayReasonEnabled WRITE setAwayReasonEnabled STORED false)
  Q_PROPERTY(bool autoAwayEnabled READ autoAwayEnabled WRITE setAutoAwayEnabled STORED false)
  Q_PROPERTY(int autoAwayTime READ autoAwayTime WRITE setAutoAwayTime STORED false)
  Q_PROPERTY(QString autoAwayReason READ autoAwayReason WRITE setAutoAwayReason STORED false)
  Q_PROPERTY(bool autoAwayReasonEnabled READ autoAwayReasonEnabled WRITE setAutoAwayReasonEnabled STORED false)
  Q_PROPERTY(bool detachAwayEnabled READ detachAwayEnabled WRITE setDetachAwayEnabled STORED false)
  Q_PROPERTY(QString detachAwayReason READ detachAwayReason WRITE setDetachAwayReason STORED false)
  Q_PROPERTY(bool detachAwayReasonEnabled READ detachAwayReasonEnabled WRITE setDetachAwayReasonEnabled STORED false)
  Q_PROPERTY(QString ident READ ident WRITE setIdent STORED false)
  Q_PROPERTY(QString kickReason READ kickReason WRITE setKickReason STORED false)
  Q_PROPERTY(QString partReason READ partReason WRITE setPartReason STORED false)
  Q_PROPERTY(QString quitReason READ quitReason WRITE setQuitReason STORED false)

  public:
    Identity(IdentityId id = 0, QObject *parent = 0);
    Identity(const Identity &other, QObject *parent = 0);
    void setToDefaults();

    bool operator==(const Identity &other);
    bool operator!=(const Identity &other);

    bool isValid() const;

    IdentityId id() const;
    QString identityName() const;
    QString realName() const;
    QStringList nicks() const;
    QString awayNick() const;
    bool awayNickEnabled() const;
    QString awayReason() const;
    bool awayReasonEnabled() const;
    bool autoAwayEnabled() const;
    int autoAwayTime() const;
    QString autoAwayReason() const;
    bool autoAwayReasonEnabled() const;
    bool detachAwayEnabled() const;
    QString detachAwayReason() const;
    bool detachAwayReasonEnabled() const;
    QString ident() const;
    QString kickReason() const;
    QString partReason() const;
    QString quitReason() const;

  public slots:
    void setId(IdentityId id);
    void setIdentityName(const QString &name);
    void setRealName(const QString &realName);
    void setNicks(const QStringList &nicks);
    void setAwayNick(const QString &awayNick);
    void setAwayNickEnabled(bool enabled);
    void setAwayReason(const QString &awayReason);
    void setAwayReasonEnabled(bool enabled);
    void setAutoAwayEnabled(bool enabled);
    void setAutoAwayTime(int time);
    void setAutoAwayReason(const QString &reason);
    void setAutoAwayReasonEnabled(bool enabled);
    void setDetachAwayEnabled(bool enabled);
    void setDetachAwayReason(const QString &reason);
    void setDetachAwayReasonEnabled(bool enabled);
    void setIdent(const QString &ident);
    void setKickReason(const QString &reason);
    void setPartReason(const QString &reason);
    void setQuitReason(const QString &reason);

    void update(const Identity &other);

  signals:
    void idSet(IdentityId id);
    void identityNameSet(const QString &name);
    void realNameSet(const QString &realName);
    void nicksSet(const QStringList &nicks);
    void awayNickSet(const QString &awayNick);
    void awayNickEnabledSet(bool);
    void awayReasonSet(const QString &awayReason);
    void awayReasonEnabledSet(bool);
    void autoAwayEnabledSet(bool);
    void autoAwayTimeSet(int);
    void autoAwayReasonSet(const QString &);
    void autoAwayReasonEnabledSet(bool);
    void detachAwayEnabledSet(bool);
    void detachAwayReasonSet(const QString &);
    void detachAwayReasonEnabledSet(bool);
    void identSet(const QString &);
    void kickReasonSet(const QString &);
    void partReasonSet(const QString &);
    void quitReasonSet(const QString &);

  private:
    IdentityId _identityId;
    QString _identityName, _realName;
    QStringList _nicks;
    QString _awayNick;
    bool _awayNickEnabled;
    QString _awayReason;
    bool _awayReasonEnabled;
    bool _autoAwayEnabled;
    int _autoAwayTime;
    QString _autoAwayReason;
    bool _autoAwayReasonEnabled;
    bool _detachAwayEnabled;
    QString _detachAwayReason;
    bool _detachAwayReasonEnabled;
    QString _ident, _kickReason, _partReason, _quitReason;

    void init();

    friend QDataStream &operator>>(QDataStream &in, Identity &identity);
};

QDataStream &operator<<(QDataStream &out, Identity identity);
QDataStream &operator>>(QDataStream &in, Identity &identity);

Q_DECLARE_METATYPE(Identity)

#endif
