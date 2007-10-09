/***************************************************************************
 *   Copyright (C) 2005-07 by The Quassel IRC Development Team             *
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
#include <QObject>
#include <QStringList>

#include "global.h"
#include "logger.h"
#include "message.h"
#include "util.h"

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

/**************************************************************************************/
BufferId::BufferId()
  : _id(0),
    _netid(0),
    _gid(0),
    _networkName(QString()),
    _bufferName(QString()) {
}

BufferId::BufferId(uint id, uint networkid, uint gid, QString net, QString buf)
  : _id(id),
    _netid(networkid),
    _gid(gid),
    _networkName(net),
    _bufferName(buf) {
}

QString BufferId::buffer() const {
  if(isChannelName(_bufferName))
    return _bufferName;
  else
    return nickFromMask(_bufferName);
}

QDataStream &operator<<(QDataStream &out, const BufferId &bufferId) {
  out << bufferId._id << bufferId._netid << bufferId._gid << bufferId._networkName.toUtf8() << bufferId._bufferName.toUtf8();
  return out;
}

QDataStream &operator>>(QDataStream &in, BufferId &bufferId) {
  QByteArray n, b;
  in >> bufferId._id >> bufferId._netid >> bufferId._gid >> n >> b;
  bufferId._networkName = QString::fromUtf8(n);
  bufferId._bufferName = QString::fromUtf8(b);
  return in;
}

uint qHash(const BufferId &bufferid) {
  return qHash(bufferid._id);
}

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

Global::RunMode Global::runMode;
QString Global::quasselDir;
