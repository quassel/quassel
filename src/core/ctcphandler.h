/***************************************************************************
 *   Copyright (C) 2005-10 by the Quassel Project                          *
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

#ifndef _CTCPHANDLER_H_
#define _CTCPHANDLER_H_

#include <QHash>
#include <QStringList>

#include "corebasichandler.h"

class CoreListIgnoreListManager;

class CtcpHandler : public CoreBasicHandler {
  Q_OBJECT

public:
  CtcpHandler(CoreNetwork *parent = 0);

  enum CtcpType {CtcpQuery, CtcpReply};

  void parse(Message::Type, const QString &prefix, const QString &target, const QByteArray &message);

  QByteArray lowLevelQuote(const QByteArray &);
  QByteArray lowLevelDequote(const QByteArray &);
  QByteArray xdelimQuote(const QByteArray &);
  QByteArray xdelimDequote(const QByteArray &);

  QByteArray pack(const QByteArray &ctcpTag, const QByteArray &message);
  void query(const QString &bufname, const QString &ctcpTag, const QString &message);
  void reply(const QString &bufname, const QString &ctcpTag, const QString &message);

public slots:
  void handleAction(CtcpType, const QString &prefix, const QString &target, const QString &param, QString &reply);
  void handleClientinfo(CtcpType, const QString &prefix, const QString &target, const QString &param, QString &reply);
  void handlePing(CtcpType, const QString &prefix, const QString &target, const QString &param, QString &reply);
  void handleTime(CtcpType, const QString &prefix, const QString &target, const QString &param, QString &reply);
  void handleVersion(CtcpType, const QString &prefix, const QString &target, const QString &param, QString &reply);

  void defaultHandler(const QString &cmd, CtcpType ctcptype, const QString &prefix, const QString &target, const QString &param, QString &reply);

private:
  void packedReply(const QString &bufname, const QList<QByteArray> &replies);

  QByteArray XDELIM;
  QHash<QByteArray, QByteArray> ctcpMDequoteHash;
  QHash<QByteArray, QByteArray> ctcpXDelimDequoteHash;
  CoreIgnoreListManager *_ignoreListManager;
};


#endif
