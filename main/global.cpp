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

#include "global.h"
#include "logger.h"
#include "core.h"

#include <QString>
#include <QStringList>
#include <QDomDocument>

extern void messageHandler(QtMsgType type, const char *msg);

Global::Global() {
  if(global) qFatal("Trying to instantiate more than one Global object!");
  qInstallMsgHandler(messageHandler);
  //initIconMap();
}

/*
void Global::setLogger(Logger *) {


};
*/

QVariant Global::getData(QString key, QVariant defval) {
  QVariant d;
  mutex.lock();
  if(data.contains(key)) d = data[key];
  else d = defval;
  mutex.unlock();
  //qDebug() << "getData("<<key<<"): " << d;
  return d;
}

QStringList Global::getKeys() {
  QStringList k;
  mutex.lock();
  k = data.keys();
  mutex.unlock();
  return k;
}

void Global::putData(QString key, QVariant d) {
  mutex.lock();
  data[key] = d;
  mutex.unlock();
  emit dataPutLocally(key);
}

void Global::updateData(QString key, QVariant d) {
  mutex.lock();
  data[key] = d;
  mutex.unlock();
  emit dataUpdatedRemotely(key);
}

/* not done yet */
void Global::initIconMap() {
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
//QIcon *Global::getIcon(QString /*symbol*/) {
  //if(symbol == "connect"

//  return 0;
//}

Global *global = 0;
Global::RunMode Global::runMode;
