/***************************************************************************
 *   Copyright (C) 2005-07 by the Quassel IRC Team                         *
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

#include "basichandler.h"

class CtcpHandler : public BasicHandler {
  Q_OBJECT

public:
  CtcpHandler(Server *parent = 0);

  enum CtcpType {CtcpQuery, CtcpReply};

  QStringList parse(CtcpType, QString, QString, QString);    

  QString dequote(QString);
  QString XdelimDequote(QString);

  QString pack(QString ctcpTag, QString message);
  void query(QString bufname, QString ctcpTag, QString message);
  void reply(QString bufname, QString ctcpTag, QString message);
  
public slots:
  void handleAction(CtcpType, QString prefix, QString target, QString param);
  void handlePing(CtcpType, QString prefix, QString target, QString param);
  void handleVersion(CtcpType, QString prefix, QString target, QString param);

  void defaultHandler(QString cmd, CtcpType ctcptype, QString prefix, QString target, QString param);

private:
  QString XDELIM;
  QHash<QString, QString> ctcpMDequoteHash;
  QHash<QString, QString> ctcpXDelimDequoteHash;    


};


#endif
