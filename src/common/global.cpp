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
#include <QDateTime>
#include <QObject>
#include <QMetaType>

#include "global.h"
#include "logger.h"
#include "message.h"
#include "identity.h"
#include "network.h"
#include "bufferinfo.h"
#include "types.h"
#include "syncableobject.h"

extern void messageHandler(QtMsgType type, const char *msg);

/* not done yet */
/*
void Global::initIconMap() {
// Do not depend on GUI in core!
  QDomDocument doc("IconMap");
  QFile file("images/iconmap.xml");
  if(!file.open(QIODevice::ReadOnly)) {
    qDebug() << "Error opening iconMap file!";
    return;
  } else if(!doc.setContent(&file)) {
    file.close();
    qDebug() << "Error parsing iconMap file!";
  } else {
    file.close();

  }
}
*/

/**
 * Retrieves an icon determined by its symbolic name. The mapping shall later
 * be performed by a theme manager or something like that.
 * @param symbol Symbol of requested icon
 * @return Pointer to a newly created QIcon
 */
//
//QIcon *Global::getIcon(QString /*symbol*/) {
  //if(symbol == "connect"

//  return 0;
//}

//! Register our custom types with Qt's Meta Object System.
/**  This makes them available for QVariant and in signals/slots, among other things.
 *
 */
void Global::registerMetaTypes() {
  // Complex types
  qRegisterMetaType<QVariant>("QVariant");
  qRegisterMetaType<Message>("Message");
  qRegisterMetaType<BufferInfo>("BufferInfo");
  qRegisterMetaType<NetworkInfo>("NetworkInfo");
  qRegisterMetaType<Identity>("Identity");
  qRegisterMetaType<Network::ConnectionState>("Network::ConnectionState");

  qRegisterMetaTypeStreamOperators<QVariant>("QVariant");
  qRegisterMetaTypeStreamOperators<Message>("Message");
  qRegisterMetaTypeStreamOperators<BufferInfo>("BufferInfo");
  qRegisterMetaTypeStreamOperators<NetworkInfo>("NetworkInfo");
  qRegisterMetaTypeStreamOperators<Identity>("Identity");
  qRegisterMetaTypeStreamOperators<qint8>("Network::ConnectionState");

  qRegisterMetaType<IdentityId>("IdentityId");
  qRegisterMetaType<BufferId>("BufferId");
  qRegisterMetaType<NetworkId>("NetworkId");
  qRegisterMetaType<UserId>("UserId");
  qRegisterMetaType<AccountId>("AccountId");
  qRegisterMetaType<MsgId>("MsgId");

  qRegisterMetaTypeStreamOperators<IdentityId>("IdentityId");
  qRegisterMetaTypeStreamOperators<BufferId>("BufferId");
  qRegisterMetaTypeStreamOperators<NetworkId>("NetworkId");
  qRegisterMetaTypeStreamOperators<UserId>("UserId");
  qRegisterMetaTypeStreamOperators<AccountId>("AccountId");
  qRegisterMetaTypeStreamOperators<MsgId>("MsgId");
}

//! This includes version.inc and possibly version.gen and sets up our version numbers.
void Global::setupVersion() {

#  include "version.inc"
#  include "version.gen"

  if(quasselGeneratedVersion.isEmpty()) {
    if(quasselCommit.isEmpty())
      quasselVersion = QString("v%1 (unknown rev)").arg(quasselBaseVersion);
    else
      quasselVersion = QString("v%1 (dist-%2, %3)").arg(quasselBaseVersion).arg(quasselCommit.left(7))
                          .arg(QDateTime::fromTime_t(quasselArchiveDate).toLocalTime().toString("yyyy-MM-dd"));
  } else {
    QStringList parts = quasselGeneratedVersion.split(':');
    quasselVersion = QString("v%1").arg(parts[0]);
    if(parts.count() >= 2) quasselVersion.append(QString(" (%1)").arg(parts[1]));
  }
  quasselBuildDate = __DATE__;
  quasselBuildTime = __TIME__;
}

// Static variables

QString Global::quasselVersion;
QString Global::quasselBaseVersion;
QString Global::quasselGeneratedVersion;
QString Global::quasselBuildDate;
QString Global::quasselBuildTime;
QString Global::quasselCommit;
uint Global::quasselArchiveDate;
uint Global::protocolVersion;
uint Global::clientNeedsProtocol;
uint Global::coreNeedsProtocol;

Global::RunMode Global::runMode;
uint Global::defaultPort;

bool Global::DEBUG;
CliParser Global::parser;
