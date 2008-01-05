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

class Identity : public QObject {
  Q_OBJECT

  Q_PROPERTY(IdentityId identityId READ id STORED false)
  Q_PROPERTY(QString identityName READ identityName WRITE setIdentityName STORED false)
  Q_PROPERTY(QString realName READ realName WRITE setRealName STORED false)
  Q_PROPERTY(QStringList nicks READ nicks WRITE setNicks STORED false)
  Q_PROPERTY(QString awayNick READ awayNick WRITE setAwayNick STORED false)
  Q_PROPERTY(QString awayReason READ awayReason WRITE setAwayReason STORED false)
  Q_PROPERTY(QString returnMessage READ returnMessage WRITE setReturnMessage STORED false)
  //Q_PROPERTY(

  public:
    Identity(IdentityId id = -1, QObject *parent = 0);
    Identity(const Identity &other, QObject *parent = 0);
    void setToDefaults();

    IdentityId id() const;
    QString identityName() const;
    QString realName() const;
    QStringList nicks() const;
    QString awayNick() const;
    QString awayReason() const;
    QString returnMessage() const;

    bool initialized() const;
    void setInitialized();

  public slots:
    void setIdentityName(const QString &name);
    void setRealName(const QString &realName);
    void setNicks(const QStringList &nicks);
    void setAwayNick(const QString &awayNick);
    void setAwayReason(const QString &awayReason);
    void setReturnMessage(const QString &returnMessage);

    void update(const Identity &other);

  signals:
    void identityNameSet(const QString &name);
    void realNameSet(const QString &realName);
    void nicksSet(const QStringList &nicks);
    void awayNickSet(const QString &awayNick);
    void awayReasonSet(const QString &awayReason);
    void returnMessageSet(const QString &returnMessage);

  private:
    bool _initialized;
    IdentityId _identityId;
    QString _identityName, _realName;
    QStringList _nicks;
    QString  _awayNick, _awayReason, _returnMessage;

    void init();

    friend QDataStream &operator>>(QDataStream &in, Identity &identity);
};

QDataStream &operator<<(QDataStream &out, const Identity &identity);
QDataStream &operator>>(QDataStream &in, Identity &identity);

Q_DECLARE_METATYPE(Identity);

#endif
