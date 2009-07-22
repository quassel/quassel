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

#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>

#include "contextmenuactionprovider.h"

#include "buffermodel.h"
#include "buffersettings.h"
#include "iconloader.h"
#include "clientidentity.h"
#include "network.h"
#include "util.h"

ContextMenuActionProvider::ContextMenuActionProvider(QObject *parent) : NetworkModelController(parent) {
  registerAction(NetworkConnect, SmallIcon("network-connect"), tr("Connect"));
  registerAction(NetworkDisconnect, SmallIcon("network-disconnect"), tr("Disconnect"));

  registerAction(BufferJoin, SmallIcon("irc-join-channel"), tr("Join"));
  registerAction(BufferPart, SmallIcon("irc-close-channel"), tr("Part"));
  registerAction(BufferRemove, tr("Delete Buffer(s)..."));
  registerAction(BufferSwitchTo, tr("Show Buffer"));

  registerAction(HideJoin, tr("Joins"), true);
  registerAction(HidePart, tr("Parts"), true);
  registerAction(HideQuit, tr("Quits"), true);
  registerAction(HideNick, tr("Nick Changes"), true);
  registerAction(HideMode, tr("Mode Changes"), true);
  registerAction(HideDayChange, tr("Day Changes"), true);
  registerAction(HideTopic, tr("Topic Changes"), true);
  registerAction(HideApplyToAll, tr("Set as Default..."));
  registerAction(HideUseDefaults, tr("Use Defaults..."));

  registerAction(JoinChannel, SmallIcon("irc-join-channel"), tr("Join Channel..."));

  registerAction(NickQuery, tr("Start Query"));
  registerAction(NickSwitchTo, tr("Show Query"));
  registerAction(NickWhois, tr("Whois"));

  registerAction(NickCtcpVersion, tr("Version"));
  registerAction(NickCtcpTime, tr("Time"));
  registerAction(NickCtcpPing, tr("Ping"));
  registerAction(NickCtcpFinger, tr("Finger"));

  registerAction(NickOp, SmallIcon("irc-operator"), tr("Give Operator Status"));
  registerAction(NickDeop, SmallIcon("irc-remove-operator"), tr("Take Operator Status"));
  registerAction(NickVoice, SmallIcon("irc-voice"), tr("Give Voice"));
  registerAction(NickDevoice, SmallIcon("irc-unvoice"), tr("Take Voice"));
  registerAction(NickKick, SmallIcon("im-kick-user"), tr("Kick From Channel"));
  registerAction(NickBan, SmallIcon("im-ban-user"), tr("Ban From Channel"));
  registerAction(NickKickBan, SmallIcon("im-ban-kick-user"), tr("Kick && Ban"));

  registerAction(HideBufferTemporarily, tr("Hide Buffer(s) Temporarily"));
  registerAction(HideBufferPermanently, tr("Hide Buffer(s) Permanently"));
  registerAction(ShowChannelList, tr("Show Channel List"));
  registerAction(ShowIgnoreList, tr("Show Ignore List"));

  QMenu *hideEventsMenu = new QMenu();
  hideEventsMenu->addAction(action(HideJoin));
  hideEventsMenu->addAction(action(HidePart));
  hideEventsMenu->addAction(action(HideQuit));
  hideEventsMenu->addAction(action(HideNick));
  hideEventsMenu->addAction(action(HideMode));
  hideEventsMenu->addAction(action(HideTopic));
  hideEventsMenu->addAction(action(HideDayChange));
  hideEventsMenu->addSeparator();
  hideEventsMenu->addAction(action(HideApplyToAll));
  hideEventsMenu->addAction(action(HideUseDefaults));
  _hideEventsMenuAction = new Action(tr("Hide Events"), 0);
  _hideEventsMenuAction->setMenu(hideEventsMenu);

  QMenu *nickCtcpMenu = new QMenu();
  nickCtcpMenu->addAction(action(NickCtcpPing));
  nickCtcpMenu->addAction(action(NickCtcpVersion));
  nickCtcpMenu->addAction(action(NickCtcpTime));
  nickCtcpMenu->addAction(action(NickCtcpFinger));
  _nickCtcpMenuAction = new Action(tr("CTCP"), 0);
  _nickCtcpMenuAction->setMenu(nickCtcpMenu);

  QMenu *nickModeMenu = new QMenu();
  nickModeMenu->addAction(action(NickOp));
  nickModeMenu->addAction(action(NickDeop));
  nickModeMenu->addAction(action(NickVoice));
  nickModeMenu->addAction(action(NickDevoice));
  nickModeMenu->addSeparator();
  nickModeMenu->addAction(action(NickKick));
  nickModeMenu->addAction(action(NickBan));
  nickModeMenu->addAction(action(NickKickBan));
  _nickModeMenuAction = new Action(tr("Actions"), 0);
  _nickModeMenuAction->setMenu(nickModeMenu);
}

ContextMenuActionProvider::~ContextMenuActionProvider() {
  _hideEventsMenuAction->menu()->deleteLater();
  _hideEventsMenuAction->deleteLater();
  _nickCtcpMenuAction->menu()->deleteLater();
  _nickCtcpMenuAction->deleteLater();
  _nickModeMenuAction->menu()->deleteLater();
  _nickModeMenuAction->deleteLater();
}

void ContextMenuActionProvider::addActions(QMenu *menu, BufferId bufId, QObject *receiver, const char *method) {
  if(!bufId.isValid())
    return;
  addActions(menu, Client::networkModel()->bufferIndex(bufId), receiver, method);
}

void ContextMenuActionProvider::addActions(QMenu *menu, const QModelIndex &index, QObject *receiver, const char *method, bool isCustomBufferView) {
  if(!index.isValid())
    return;
  addActions(menu, QList<QModelIndex>() << index, 0, QString(), receiver, method, isCustomBufferView);
}

void ContextMenuActionProvider::addActions(QMenu *menu, MessageFilter *filter, BufferId msgBuffer, QObject *receiver, const char *slot) {
  addActions(menu, filter, msgBuffer, QString(), receiver, slot);
}

void ContextMenuActionProvider::addActions(QMenu *menu, MessageFilter *filter, BufferId msgBuffer, const QString &chanOrNick, QObject *receiver, const char *method) {
  if(!filter)
    return;
  addActions(menu, QList<QModelIndex>() << Client::networkModel()->bufferIndex(msgBuffer), filter, chanOrNick, receiver, method, false);
}

void ContextMenuActionProvider::addActions(QMenu *menu, const QList<QModelIndex> &indexList, QObject *receiver,  const char *method, bool isCustomBufferView) {
  addActions(menu, indexList, 0, QString(), receiver, method, isCustomBufferView);
}

// add a list of actions sensible for the current item(s)
void ContextMenuActionProvider::addActions(QMenu *menu,
                                            const QList<QModelIndex> &indexList_,
                                            MessageFilter *filter_,
                                            const QString &contextItem_,
                                            QObject *receiver_,
                                            const char *method_,
                                            bool isCustomBufferView)
{
  if(!indexList_.count())
    return;

  setIndexList(indexList_);
  setMessageFilter(filter_);
  setContextItem(contextItem_);
  setSlot(receiver_, method_);

  if(!messageFilter()) {
    // this means we are in a BufferView (or NickView) rather than a ChatView

    // first index in list determines the menu type (just in case we have both buffers and networks selected, for example)
    QModelIndex index = indexList().at(0);
    NetworkModel::ItemType itemType = static_cast<NetworkModel::ItemType>(index.data(NetworkModel::ItemTypeRole).toInt());

    switch(itemType) {
      case NetworkModel::NetworkItemType:
        addNetworkItemActions(menu, index);
        break;
      case NetworkModel::BufferItemType:
        addBufferItemActions(menu, index, isCustomBufferView);
        break;
      case NetworkModel::IrcUserItemType:
        addIrcUserActions(menu, index);
        break;
      default:
        return;

    }
  } else {
    // ChatView actions
    if(contextItem().isEmpty()) {
      // a) query buffer: handle like ircuser
      // b) general chatview: handle like channel iff it displays a single buffer
      // NOTE stuff breaks probably with merged buffers, need to rework a lot around here then
      if(messageFilter()->containedBuffers().count() == 1) {
        // we can handle this like a single bufferItem
        QModelIndex index = Client::networkModel()->bufferIndex(messageFilter()->containedBuffers().values().at(0));
        setIndexList(index);
        addBufferItemActions(menu, index);
        return;
      } else {
        // TODO: actions for merged buffers... _indexList contains the index of the message we clicked on

      }
    } else {
      // context item = chan or nick, _indexList = buf where the msg clicked on originated
      if(isChannelName(contextItem())) {
        QModelIndex msgIdx = indexList().at(0);
        if(!msgIdx.isValid())
          return;
        NetworkId networkId = msgIdx.data(NetworkModel::NetworkIdRole).value<NetworkId>();
        BufferId bufId = Client::networkModel()->bufferId(networkId, contextItem());
        if(bufId.isValid()) {
          QModelIndex targetIdx = Client::networkModel()->bufferIndex(bufId);
          setIndexList(targetIdx);
          addAction(BufferJoin, menu, targetIdx, InactiveState);
          addAction(BufferSwitchTo, menu, targetIdx, ActiveState);
        } else
          addAction(JoinChannel, menu);
      } else {
        // TODO: actions for a nick
      }
    }
  }
}

void ContextMenuActionProvider::addNetworkItemActions(QMenu *menu, const QModelIndex &index) {
  NetworkId networkId = index.data(NetworkModel::NetworkIdRole).value<NetworkId>();
  if(!networkId.isValid())
    return;
  const Network *network = Client::network(networkId);
  Q_CHECK_PTR(network);
  if(!network)
    return;

  addAction(NetworkConnect, menu, network->connectionState() == Network::Disconnected);
  addAction(NetworkDisconnect, menu, network->connectionState() != Network::Disconnected);
  menu->addSeparator();
  addAction(ShowChannelList, menu, index, ActiveState);
  addAction(JoinChannel, menu, index, ActiveState);

}

void ContextMenuActionProvider::addBufferItemActions(QMenu *menu, const QModelIndex &index, bool isCustomBufferView) {
  BufferInfo bufferInfo = index.data(NetworkModel::BufferInfoRole).value<BufferInfo>();

  menu->addSeparator();
  switch(bufferInfo.type()) {
    case BufferInfo::ChannelBuffer:
      addAction(BufferJoin, menu, index, InactiveState);
      addAction(BufferPart, menu, index, ActiveState);
      menu->addSeparator();
      addHideEventsMenu(menu, bufferInfo.bufferId());
      menu->addSeparator();
      addAction(HideBufferTemporarily, menu, isCustomBufferView);
      addAction(HideBufferPermanently, menu, isCustomBufferView);
      addAction(BufferRemove, menu, index, InactiveState);
      break;

    case BufferInfo::QueryBuffer:
    {
      //IrcUser *ircUser = qobject_cast<IrcUser *>(index.data(NetworkModel::IrcUserRole).value<QObject *>());
      //if(ircUser) {
        addIrcUserActions(menu, index);
        menu->addSeparator();
      //}
      addHideEventsMenu(menu, bufferInfo.bufferId());
      menu->addSeparator();
      addAction(HideBufferTemporarily, menu, isCustomBufferView);
      addAction(HideBufferPermanently, menu, isCustomBufferView);
      addAction(BufferRemove, menu, index);
      break;
    }

    default:
      addAction(HideBufferTemporarily, menu, isCustomBufferView);
      addAction(HideBufferPermanently, menu, isCustomBufferView);
  }
}

void ContextMenuActionProvider::addIrcUserActions(QMenu *menu, const QModelIndex &index) {
  // this can be called: a) as a nicklist context menu (index has IrcUserItemType)
  //                     b) as a query buffer context menu (index has BufferItemType and is a QueryBufferItem)
  //                     c) right-click in a query chatview (same as b), index will be the corresponding QueryBufferItem)
  //                     d) right-click on some nickname (_contextItem will be non-null, _filter -> chatview, index -> message buffer)

  if(contextItem().isNull()) {
    // cases a, b, c
    bool haveQuery = indexList().count() == 1 && findQueryBuffer(index).isValid();
    NetworkModel::ItemType itemType = static_cast<NetworkModel::ItemType>(index.data(NetworkModel::ItemTypeRole).toInt());
    addAction(_nickModeMenuAction, menu, itemType == NetworkModel::IrcUserItemType);
    addAction(_nickCtcpMenuAction, menu);
    menu->addSeparator();
    addAction(NickQuery, menu, itemType == NetworkModel::IrcUserItemType && !haveQuery && indexList().count() == 1);
    addAction(NickSwitchTo, menu, itemType == NetworkModel::IrcUserItemType && haveQuery);
    menu->addSeparator();
    addAction(NickWhois, menu, true);

  } else if(!contextItem().isEmpty() && messageFilter()) {
    // case d
    // TODO

  }
}

Action * ContextMenuActionProvider::addAction(ActionType type , QMenu *menu, const QModelIndex &index, ItemActiveStates requiredActiveState) {
  return addAction(action(type), menu, checkRequirements(index, requiredActiveState));
}

Action * ContextMenuActionProvider::addAction(Action *action , QMenu *menu, const QModelIndex &index, ItemActiveStates requiredActiveState) {
  return addAction(action, menu, checkRequirements(index, requiredActiveState));
}

Action * ContextMenuActionProvider::addAction(ActionType type , QMenu *menu, bool condition) {
  return addAction(action(type), menu, condition);
}

Action * ContextMenuActionProvider::addAction(Action *action , QMenu *menu, bool condition) {
  if(condition) {
    menu->addAction(action);
    action->setVisible(true);
  } else {
    action->setVisible(false);
  }
  return action;
}

void ContextMenuActionProvider::addHideEventsMenu(QMenu *menu, BufferId bufferId) {
  if(BufferSettings(bufferId).hasFilter())
    addHideEventsMenu(menu, BufferSettings(bufferId).messageFilter());
  else
    addHideEventsMenu(menu);
}

void ContextMenuActionProvider::addHideEventsMenu(QMenu *menu, MessageFilter *msgFilter) {
  if(BufferSettings(msgFilter->idString()).hasFilter())
    addHideEventsMenu(menu, BufferSettings(msgFilter->idString()).messageFilter());
  else
    addHideEventsMenu(menu);
}

void ContextMenuActionProvider::addHideEventsMenu(QMenu *menu, int filter) {
  action(HideApplyToAll)->setEnabled(filter != -1);
  action(HideUseDefaults)->setEnabled(filter != -1);
  if(filter == -1)
    filter = BufferSettings().messageFilter();

  action(HideJoin)->setChecked(filter & Message::Join);
  action(HidePart)->setChecked(filter & Message::Part);
  action(HideQuit)->setChecked(filter & Message::Quit);
  action(HideNick)->setChecked(filter & Message::Nick);
  action(HideMode)->setChecked(filter & Message::Mode);
  action(HideDayChange)->setChecked(filter & Message::DayChange);
  action(HideTopic)->setChecked(filter & Message::Topic);

  menu->addAction(_hideEventsMenuAction);
}
