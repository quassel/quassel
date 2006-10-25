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

#include "util.h"

#include <QtCore>

QString nickFromMask(QString mask) {
  return mask.section('!', 0, 0);
}

QString userFromMask(QString mask) {
  QString userhost = mask.section('!', 1);
  if(userhost.isEmpty()) return QString();
  return userhost.section('@', 0, 0);
}

QString hostFromMask(QString mask) {
  QString userhost = mask.section('!', 1);
  if(userhost.isEmpty()) return QString();
  return userhost.section('@', 1);
}

void writeDataToDevice(QIODevice *dev, const QVariant &item) {
  QByteArray block;
  QDataStream out(&block, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_4_1);
  out << (quint32)0 << item;
  out.device()->seek(0);
  out << (quint32)(block.size() - sizeof(quint32));
  dev->write(block);
}

bool readDataFromDevice(QIODevice *dev, quint32 &blockSize, QVariant &item) {
  QDataStream in(dev);
  in.setVersion(QDataStream::Qt_4_1);

  if(blockSize == 0) {
    if(dev->bytesAvailable() < (int)sizeof(quint32)) return false;
    in >> blockSize;
  }
  if(dev->bytesAvailable() < blockSize) return false;
  in >> item;
  return true;
}
