/***************************************************************************
 *   Copyright (C) 2005/06 by The Quassel Team                             *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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

#ifndef _SYNCHRONIZER_H_
#define _SYNCHRONIZER_H_

class SignalProxy;

#include <QObject>;
#include <QList>;
#include <QString>;
#include <QVariantMap>;
#include <QMetaMethod>;
#include <QPointer>

class Synchronizer : public QObject {
  Q_OBJECT

public:
  Synchronizer(QObject *parent, SignalProxy *signalproxy);
  
  bool initialized() const;
  SignalProxy *proxy() const;

  QVariantMap initData() const;
  void setInitData(const QVariantMap &properties);

public slots:
  void synchronizeClients() const;
  void recvInitData(QVariantMap properties);
  void parentChangedName();
  
signals:
  void requestSync() const;
  void sendInitData(QVariantMap properties) const;
  void initDone();
  
private:
  bool _initialized;
  QPointer<SignalProxy> _signalproxy;
  
  QString signalPrefix() const;
  QString initSignal() const;
  QString requestSyncSignal() const;
  
  QString methodBaseName(const QMetaMethod &method) const;
  bool methodsMatch(const QMetaMethod &signal, const QMetaMethod &slot) const;

  QList<QMetaProperty> parentProperties() const;
  QList<QMetaMethod> parentSlots() const;
  QList<QMetaMethod> parentSignals() const;
  
  QList<QMetaMethod> getMethodByName(const QString &methodname);
  
  void attach();
  void attachAsSlave();
  void attachAsMaster();
			  
  bool setInitValue(const QString &property, const QVariant &value);
};

#endif
