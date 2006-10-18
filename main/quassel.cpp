/***************************************************************************
 *   Copyright (C) 2005 by The Quassel Team                                *
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

#include "quassel.h"
#include "logger.h"
//#include "proxy.h"
#include "core.h"

#include <QString>
#include <QDomDocument>

extern void messageHandler(QtMsgType type, const char *msg);

Quassel * Quassel::init() {
  if(quassel) return quassel;
  qInstallMsgHandler(messageHandler);
  quassel = new Quassel();
  //initIconMap();
  return quassel;
}

/*
void Quassel::setLogger(Logger *) {


};
*/

QVariant Quassel::getData(QString key) {
  mutex.lock();
  QVariant d = data[key];
  mutex.unlock();
  qDebug() << "getData("<<key<<"): " << d;
  return d;
}

void Quassel::putData(QString key, const QVariant &d) {
  mutex.lock();
  data[key] = d;
  mutex.unlock();
  emit dataChanged(key, d);
  qDebug() << "putData("<<key<<"): " << d;
  qDebug() << "data: " << data;
}

/* not done yet */
void Quassel::initIconMap() {
// Do not depend on GUI in core!
/*
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
*/
}


/**
 * Retrieves an icon determined by its symbolic name. The mapping shall later
 * be performed by a theme manager or something like that.
 * @param symbol Symbol of requested icon
 * @return Pointer to a newly created QIcon
 */
//
//QIcon *Quassel::getIcon(QString /*symbol*/) {
  //if(symbol == "connect"

//  return 0;
//}

Quassel *quassel = 0;
Quassel::RunMode Quassel::runMode;
QMutex Quassel::mutex;
QHash<QString, QVariant> Quassel::data;
