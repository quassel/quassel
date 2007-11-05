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
