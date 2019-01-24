/***************************************************************************
 *   Copyright (C) 2005-2019 by the Quassel Project                        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "toolbaractionprovider.h"

#include <QMenu>
#include <QToolBar>

#include "icon.h"

ToolBarActionProvider::ToolBarActionProvider(QObject* parent)
    : NetworkModelController(parent)
{
    registerAction(NetworkConnectAllWithDropdown, icon::get("network-connect"), tr("Connect"))->setToolTip(tr("Connect to IRC"));
    registerAction(NetworkDisconnectAllWithDropdown, icon::get("network-disconnect"), tr("Disconnect"))->setToolTip(tr("Disconnect from IRC"));
    registerAction(NetworkConnectAll, tr("Connect to all"));
    registerAction(NetworkDisconnectAll, tr("Disconnect from all"));

    registerAction(BufferPart, icon::get("irc-close-channel"), tr("Part"))->setToolTip(tr("Leave currently selected channel"));
    registerAction(JoinChannel, icon::get("irc-join-channel"), tr("Join"))->setToolTip(tr("Join a channel"));

    registerAction(NickQuery, icon::get("mail-message-new"), tr("Query"))->setToolTip(tr("Start a private conversation"));  // fix icon
    registerAction(NickWhois, icon::get("im-user"), tr("Whois"))->setToolTip(tr("Request user information"));               // fix icon

    registerAction(NickOp, icon::get("irc-operator"), tr("Op"))->setToolTip(tr("Give operator privileges to user"));
    registerAction(NickDeop, icon::get("irc-remove-operator"), tr("Deop"))->setToolTip(tr("Take operator privileges from user"));
    registerAction(NickVoice, icon::get("irc-voice"), tr("Voice"))->setToolTip(tr("Give voice to user"));
    registerAction(NickDevoice, icon::get("irc-unvoice"), tr("Devoice"))->setToolTip(tr("Take voice from user"));
    registerAction(NickKick, icon::get("im-kick-user"), tr("Kick"))->setToolTip(tr("Remove user from channel"));
    registerAction(NickBan, icon::get("im-ban-user"), tr("Ban"))->setToolTip(tr("Ban user from channel"));
    registerAction(NickKickBan, icon::get("im-ban-kick-user"), tr("Kick/Ban"))->setToolTip(tr("Remove and ban user from channel"));

    _networksConnectMenu = new QMenu();
    _networksConnectMenu->setSeparatorsCollapsible(false);
    _networksConnectMenu->addSeparator();
    _networksConnectMenu->addAction(action(NetworkConnectAll));
    action(NetworkConnectAllWithDropdown)->setMenu(_networksConnectMenu);
    action(NetworkConnectAllWithDropdown)->setEnabled(false);

    _networksDisconnectMenu = new QMenu();
    _networksDisconnectMenu->setSeparatorsCollapsible(false);
    _networksDisconnectMenu->addSeparator();
    _networksDisconnectMenu->addAction(action(NetworkDisconnectAll));
    action(NetworkDisconnectAllWithDropdown)->setMenu(_networksDisconnectMenu);
    action(NetworkDisconnectAllWithDropdown)->setEnabled(false);

    connect(Client::instance(), &Client::networkCreated, this, &ToolBarActionProvider::networkCreated);
    connect(Client::instance(), &Client::networkRemoved, this, &ToolBarActionProvider::networkRemoved);

    updateStates();
}

void ToolBarActionProvider::disconnectedFromCore()
{
    _currentBuffer = QModelIndex();
    updateStates();
    NetworkModelController::disconnectedFromCore();
}

void ToolBarActionProvider::updateStates()
{
    action(BufferPart)
        ->setEnabled(_currentBuffer.isValid() && _currentBuffer.data(NetworkModel::BufferTypeRole) == BufferInfo::ChannelBuffer
                     && _currentBuffer.data(NetworkModel::ItemActiveRole).toBool());
}

void ToolBarActionProvider::addActions(QToolBar* bar, ToolBarType type)
{
    switch (type) {
    case MainToolBar:
        bar->addAction(action(NetworkConnectAllWithDropdown));
        bar->addAction(action(NetworkDisconnectAllWithDropdown));
        bar->addAction(action(JoinChannel));
        bar->addAction(action(BufferPart));
        break;
    case NickToolBar:
        bar->addAction(action(NickQuery));
        bar->addAction(action(NickWhois));
        bar->addSeparator();
        bar->addAction(action(NickOp));
        bar->addAction(action(NickDeop));
        bar->addAction(action(NickVoice));
        bar->addAction(action(NickDevoice));
        bar->addAction(action(NickKick));
        bar->addAction(action(NickBan));
        bar->addAction(action(NickKickBan));
        break;
    default:
        return;
    }
}

void ToolBarActionProvider::onCurrentBufferChanged(const QModelIndex& index)
{
    _currentBuffer = index;
    updateStates();
}

void ToolBarActionProvider::onNickSelectionChanged(const QModelIndexList& indexList)
{
    _selectedNicks = indexList;
    updateStates();
}

// override those to set indexes right
void ToolBarActionProvider::handleNetworkAction(ActionType type, QAction* action)
{
    setIndexList(_currentBuffer);
    NetworkModelController::handleNetworkAction(type, action);
}

void ToolBarActionProvider::handleBufferAction(ActionType type, QAction* action)
{
    setIndexList(_currentBuffer);
    NetworkModelController::handleBufferAction(type, action);
}

void ToolBarActionProvider::handleNickAction(ActionType type, QAction* action)
{
    setIndexList(_selectedNicks);
    NetworkModelController::handleNickAction(type, action);
}

void ToolBarActionProvider::handleGeneralAction(ActionType type, QAction* action)
{
    setIndexList(_currentBuffer);
    NetworkModelController::handleGeneralAction(type, action);
}

void ToolBarActionProvider::networkCreated(NetworkId id)
{
    const Network* net = Client::network(id);
    Action* act = new Action(net->networkName(), this);
    _networkActions[id] = act;
    act->setObjectName(QString("NetworkAction-%1").arg(id.toInt()));
    act->setData(QVariant::fromValue<NetworkId>(id));
    connect(net, &Network::updatedRemotely, this, [this]() { networkUpdated(); });
    connect(act, &QAction::triggered, this, &ToolBarActionProvider::connectOrDisconnectNet);
    networkUpdated(net);
}

void ToolBarActionProvider::networkRemoved(NetworkId id)
{
    Action* act = _networkActions.take(id);
    if (act)
        act->deleteLater();
}

void ToolBarActionProvider::networkUpdated(const Network* net)
{
    if (!net)
        net = qobject_cast<const Network*>(sender());
    if (!net)
        return;
    Action* act = _networkActions.value(net->networkId());
    if (!act)
        return;

    _networksConnectMenu->removeAction(act);
    _networksDisconnectMenu->removeAction(act);

    QMenu* newMenu = net->connectionState() != Network::Disconnected ? _networksDisconnectMenu : _networksConnectMenu;
    act->setText(net->networkName());

    const int lastidx = newMenu->actions().count() - 2;
    QAction* beforeAction = newMenu->actions().at(lastidx);
    for (int i = 0; i < newMenu->actions().count() - 2; i++) {
        QAction* action = newMenu->actions().at(i);
        if (net->networkName().localeAwareCompare(action->text()) < 0) {
            beforeAction = action;
            break;
        }
    }
    newMenu->insertAction(beforeAction, act);

    action(NetworkConnectAllWithDropdown)->setEnabled(_networksConnectMenu->actions().count() > 2);
    action(NetworkDisconnectAllWithDropdown)->setEnabled(_networksDisconnectMenu->actions().count() > 2);
    action(JoinChannel)->setEnabled(_networksDisconnectMenu->actions().count() > 2);
}

void ToolBarActionProvider::connectOrDisconnectNet()
{
    auto* act = qobject_cast<QAction*>(sender());
    if (!act)
        return;
    const Network* net = Client::network(act->data().value<NetworkId>());
    if (!net)
        return;

    if (net->connectionState() == Network::Disconnected)
        net->requestConnect();
    else
        net->requestDisconnect();
}

// void ToolBarActionProvider::
