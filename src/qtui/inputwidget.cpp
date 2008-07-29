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

#include "ircuser.h"
#include "client.h"
#include "networkmodel.h"
#include "jumpkeyhandler.h"
#include "qtuisettings.h"


InputWidget::InputWidget(QWidget *parent)
  : AbstractItemView(parent),
    _networkId(0)
{
  ui.setupUi(this);
  connect(ui.inputEdit, SIGNAL(sendText(QString)), this, SLOT(sendText(QString)));
  connect(ui.ownNick, SIGNAL(activated(QString)), this, SLOT(changeNick(QString)));
  connect(this, SIGNAL(userInput(BufferInfo, QString)), Client::instance(), SIGNAL(sendInput(BufferInfo, QString)));
  setFocusProxy(ui.inputEdit);

  ui.ownNick->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  ui.ownNick->installEventFilter(new MouseWheelFilter(this));
  ui.inputEdit->installEventFilter(new JumpKeyHandler(this));

  QtUiSettings s;
  bool useInputLineFont = s.value("UseInputLineFont", QVariant(false)).toBool();
  if(useInputLineFont) {
    ui.inputEdit->setFont(s.value("InputLineFont").value<QFont>());
  }
}

InputWidget::~InputWidget() {
}

void InputWidget::currentChanged(const QModelIndex &current, const QModelIndex &previous) {
  if(current.data(NetworkModel::BufferInfoRole) == previous.data(NetworkModel::BufferInfoRole))
    return;

  const Network *net = Client::networkModel()->networkByIndex(current);
  setNetwork(net);
  updateNickSelector();
  updateEnabledState();
}

void InputWidget::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight) {
  QItemSelectionRange changedArea(topLeft, bottomRight);
  if(changedArea.contains(selectionModel()->currentIndex())) {
    updateEnabledState();
  }
};

void InputWidget::updateEnabledState() {
  QModelIndex currentIndex = selectionModel()->currentIndex();
  
  const Network *net = Client::networkModel()->networkByIndex(currentIndex);
  bool enabled = false;
  if(net) {
    // disable inputline if it's a channelbuffer we parted from or...
    enabled = (currentIndex.data(NetworkModel::ItemActiveRole).value<bool>() || (currentIndex.data(NetworkModel::BufferTypeRole).toInt() != BufferInfo::ChannelBuffer));
    // ... if we're not connected to the network at all
    enabled &= net->isConnected();
  }
  ui.inputEdit->setEnabled(enabled);
}

const Network *InputWidget::currentNetwork() const {
  return Client::network(_networkId);
}

BufferInfo InputWidget::currentBufferInfo() const {
  return selectionModel()->currentIndex().data(NetworkModel::BufferInfoRole).value<BufferInfo>();
};

void InputWidget::setNetwork(const Network *network) {
  if(!network || _networkId == network->networkId())
    return;

  const Network *previousNet = Client::network(_networkId);
  if(previousNet) {
    disconnect(previousNet, 0, this, 0);
    if(previousNet->me())
      disconnect(previousNet->me(), 0, this, 0);
  }

  if(network) {
    _networkId = network->networkId();
    connect(network, SIGNAL(identitySet(IdentityId)), this, SLOT(setIdentity(IdentityId)));
    if(network->me()) {
      connect(network->me(), SIGNAL(nickSet(QString)), this, SLOT(updateNickSelector()));
      connect(network->me(), SIGNAL(userModesSet(QString)), this, SLOT(updateNickSelector()));
      connect(network->me(), SIGNAL(userModesAdded(QString)), this, SLOT(updateNickSelector()));
      connect(network->me(), SIGNAL(userModesRemoved(QString)), this, SLOT(updateNickSelector()));
    }
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

  if(net->me() && nickIdx < nicks.count())
    nicks[nickIdx] = net->myNick() + QString(" (+%1)").arg(net->me()->userModes());
      
  ui.ownNick->addItems(nicks);
  ui.ownNick->setCurrentIndex(nickIdx);
}

void InputWidget::changeNick(const QString &newNick) const {
  const Network *net = currentNetwork();
  if(!net || net->isMyNick(newNick))
    return;
  emit userInput(currentBufferInfo(), QString("/nick %1").arg(newNick));
}

void InputWidget::sendText(QString text) {
  emit userInput(currentBufferInfo(), text);
}


// MOUSE WHEEL FILTER
MouseWheelFilter::MouseWheelFilter(QObject *parent)
  : QObject(parent)
{
}

bool MouseWheelFilter::eventFilter(QObject *obj, QEvent *event) {
  if(event->type() != QEvent::Wheel)
    return QObject::eventFilter(obj, event);
  else
    return true;
}
