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

#include "global.h"
#include "logger.h"
#include "core.h"
#include "message.h"
#include "util.h"

#include <QtCore>
#include <QDomDocument>

extern void messageHandler(QtMsgType type, const char *msg);

Global *Global::instanceptr = 0;

Global * Global::instance() {
  if(instanceptr) return instanceptr;
  return instanceptr = new Global();
}

void Global::destroy() {
  delete instanceptr;
  instanceptr = 0;
}

Global::Global() {
  qInstallMsgHandler(messageHandler);
  qRegisterMetaType<Message>("Message");
  qRegisterMetaTypeStreamOperators<Message>("Message");
  qRegisterMetaType<BufferId>("BufferId");
  qRegisterMetaTypeStreamOperators<BufferId>("BufferId");

  guiUser = 0;
}

Global::~Global() {


}

void Global::setGuiUser(UserId uid) {
  guiUser = uid;
}

QVariant Global::data(QString key, QVariant defval) {
  return data(guiUser, key, defval);
}

QVariant Global::data(UserId uid, QString key, QVariant defval) {
  QVariant d;
  mutex.lock();
  if(instance()->datastore[uid].contains(key)) d = instance()->datastore[uid][key];
  else d = defval;
  mutex.unlock();
  //qDebug() << "getData("<<key<<"): " << d;
  return d;
}

QStringList Global::keys() {
  return keys(guiUser);
}

QStringList Global::keys(UserId uid) {
  QStringList k;
  mutex.lock();
  k = instance()->datastore[uid].keys();
  mutex.unlock();
  return k;
}

void Global::putData(QString key, QVariant d) {
  putData(guiUser, key, d);
}

void Global::putData(UserId uid, QString key, QVariant d) {
  mutex.lock();
  instance()->datastore[uid][key] = d;
  mutex.unlock();
  emit instance()->dataPutLocally(uid, key);
}

void Global::updateData(QString key, QVariant d) {
  updateData(guiUser, key, d);
}

void Global::updateData(UserId uid, QString key, QVariant d) {
  mutex.lock();
  instance()->datastore[uid][key] = d;
  mutex.unlock();
  emit instance()->dataUpdatedRemotely(uid, key);
}

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



BufferId::BufferId(uint _id, QString _net, QString _buf, uint _gid) : id(_id), gid(_gid), net(_net), buf(_buf) {


}

QString BufferId::buffer() const {
  if(isChannelName(buf)) return buf;
  else return nickFromMask(buf);
}

QDataStream &operator<<(QDataStream &out, const BufferId &bufferId) {
  out << bufferId.id << bufferId.gid << bufferId.net.toUtf8() << bufferId.buf.toUtf8();
}

QDataStream &operator>>(QDataStream &in, BufferId &bufferId) {
  QByteArray n, b;
  BufferId i;
  in >> bufferId.id >> bufferId.gid >> n >> b;
  bufferId.net = QString::fromUtf8(n);
  bufferId.buf = QString::fromUtf8(b);
}

uint qHash(const BufferId &bid) {
  return qHash(bid.id);
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

QMutex Global::mutex;
Global::RunMode Global::runMode;
UserId Global::guiUser;
QString Global::quasselDir;
