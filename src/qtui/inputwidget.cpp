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

#include "inputwidget.h"

#include "client.h"
#include "networkmodel.h"

InputWidget::InputWidget(QWidget *parent)
  : QWidget(parent),
    validBuffer(false),
    _bufferModel(0),
    _selectionModel(0)
{
  ui.setupUi(this);
  connect(ui.inputEdit, SIGNAL(returnPressed()), this, SLOT(enterPressed()));
  connect(ui.ownNick, SIGNAL(activated(QString)), this, SLOT(changeNick(QString)));
  connect(this, SIGNAL(userInput(BufferInfo, QString)), Client::instance(), SIGNAL(sendInput(BufferInfo, QString)));
  setFocusProxy(ui.inputEdit);
}

InputWidget::~InputWidget() {
}

void InputWidget::setModel(BufferModel *bufferModel) {
  _bufferModel = bufferModel;
}

void InputWidget::setSelectionModel(QItemSelectionModel *selectionModel) {
  if(_selectionModel) {
    disconnect(_selectionModel, 0, this, 0);
  }
  _selectionModel = selectionModel;
  connect(selectionModel, SIGNAL(currentChanged(QModelIndex, QModelIndex)),
	  this, SLOT(currentChanged(QModelIndex, QModelIndex)));
}

void InputWidget::currentChanged(const QModelIndex &current, const QModelIndex &previous) {
  Q_UNUSED(previous);

  validBuffer = current.isValid();

  if(!validBuffer)
    return;
  
  QVariant variant;
  variant = current.data(NetworkModel::BufferInfoRole);
  if(!variant.isValid())
    return;

  currentBufferInfo  = current.data(NetworkModel::BufferInfoRole).value<BufferInfo>();
  setNetwork(Client::networkModel()->networkByIndex(current));
  updateNickSelector();
  ui.inputEdit->setEnabled(current.data(NetworkModel::ItemActiveRole).value<bool>());
}

const Network *InputWidget::currentNetwork() const {
  if(!validBuffer)
    return 0;

  return Client::network(_networkId);
}

void InputWidget::setNetwork(const Network *network) {
  if(_networkId == network->networkId())
    return;

  const Network *previousNet = Client::network(_networkId);
  if(previousNet)
    disconnect(previousNet, 0, this, 0);

  if(network) {
    _networkId = network->networkId();
    connect(network, SIGNAL(myNickSet(QString)),
	    this, SLOT(updateNickSelector()));
    connect(network, SIGNAL(identitySet(IdentityId)),
	    this, SLOT(setIdentity(IdentityId)));
  }
  setIdentity(network->identity());
}

void InputWidget::setIdentity(const IdentityId &identityId) {
  if(_identityId == identityId)
    return;

  const Identity *previousIdentity = Client::identity(_identityId);
  if(previousIdentity)
    disconnect(previousIdentity, 0, this, 0);

  const Identity *identity = Client::identity(identityId);
  if(identity) {
    _identityId = identityId;
    connect(identity, SIGNAL(nicksSet(QStringList)),
	    this, SLOT(updateNickSelector()));
  }
  updateNickSelector();
}

void InputWidget::updateNickSelector() const {
  ui.ownNick->clear();

  const Network *net = currentNetwork();
  if(!net)
    return;

  const Identity *identity = Client::identity(net->identity());
  if(!identity) {
    qWarning() << "InputWidget::updateNickSelector(): can't find Identity for Network" << net->networkId();
    return;
  }

  int nickIdx;
  QStringList nicks = identity->nicks();
  if((nickIdx = nicks.indexOf(net->myNick())) == -1) {
    nicks.prepend(net->myNick());
    nickIdx = 0;
  }
  
  ui.ownNick->addItems(nicks);
  ui.ownNick->setCurrentIndex(nickIdx);
}

void InputWidget::changeNick(const QString &newNick) const {
  const Network *net = currentNetwork();
  if(!net || net->isMyNick(newNick))
    return;
  emit userInput(currentBufferInfo, QString("/nick %1").arg(newNick));
}

void InputWidget::enterPressed() {
  QStringList lines = ui.inputEdit->text().split('\n', QString::SkipEmptyParts);
  foreach(QString msg, lines) {
    if(msg.isEmpty()) continue;
    emit userInput(currentBufferInfo, msg);
  }
  ui.inputEdit->clear();
}

