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
#include "network.h"
#include "identity.h"

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
  updateNickSelector();

  ui.inputEdit->setEnabled(current.data(NetworkModel::ItemActiveRole).value<bool>());
}

const Network *InputWidget::currentNetwork() const {
  if(!validBuffer)
    return 0;

  return Client::network(currentBufferInfo.networkId());
}

void InputWidget::updateNickSelector() const {
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
  
  ui.ownNick->clear();
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

