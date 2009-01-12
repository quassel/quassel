/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
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

#include "jumpkeyhandler.h"

#include <QKeyEvent>

#include "client.h"
#include "networkmodel.h"
#include "buffermodel.h"

JumpKeyHandler::JumpKeyHandler(QObject *parent)
  : QObject(parent),
#ifdef Q_WS_MAC
    bindModifier(Qt::ControlModifier | Qt::AltModifier),
    jumpModifier(Qt::ControlModifier)
#else
    bindModifier(Qt::ControlModifier),
    jumpModifier(Qt::AltModifier)
#endif
{
}

bool JumpKeyHandler::eventFilter(QObject *obj, QEvent *event) {
  if(event->type() != QEvent::KeyPress)
    return QObject::eventFilter(obj, event);

  QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

  const int key = keyEvent->key();
  
  if(key < Qt::Key_1 || Qt::Key_9 < key)
    return QObject::eventFilter(obj, event);
  
  if(keyEvent->modifiers() ==  bindModifier) {
    bindKey(key);
    return true;
  }
  
  if(keyEvent->modifiers() == jumpModifier) {
    jumpKey(key);
    return true;
  }

  return false;
}
  
void JumpKeyHandler::bindKey(int key) {
  QModelIndex bufferIdx = Client::bufferModel()->standardSelectionModel()->currentIndex();
  NetworkId netId = bufferIdx.data(NetworkModel::NetworkIdRole).value<NetworkId>();
  const Network *net = Client::network(netId);
  if(!net)
    return;
  
  QString bufferName = bufferIdx.sibling(bufferIdx.row(), 0).data().toString();
  BufferId bufferId = bufferIdx.data(NetworkModel::BufferIdRole).value<BufferId>();
  
  _keyboardJump[key] = bufferId;
  CoreAccountSettings().setJumpKeyMap(_keyboardJump);
  //emit statusMessage(tr("Bound Buffer %1::%2 to Key %3").arg(net->networkName()).arg(bufferName).arg(key - Qt::Key_0), 10000);
  //statusBar()->showMessage(tr("Bound Buffer %1::%2 to Key %3").arg(net->networkName()).arg(bufferName).arg(key - Qt::Key_0), 10000);
}

void JumpKeyHandler::jumpKey(int key) {
  if(key < Qt::Key_0 || Qt::Key_9 < key)
    return;

  if(_keyboardJump.isEmpty())
    _keyboardJump = CoreAccountSettings().jumpKeyMap();

  if(!_keyboardJump.contains(key))
    return;

  QModelIndex source_bufferIdx = Client::networkModel()->bufferIndex(_keyboardJump[key]);
  QModelIndex bufferIdx = Client::bufferModel()->mapFromSource(source_bufferIdx);

  if(bufferIdx.isValid())
    Client::bufferModel()->standardSelectionModel()->setCurrentIndex(bufferIdx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
  
}
