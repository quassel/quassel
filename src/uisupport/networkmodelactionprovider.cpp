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

#include "networkmodelactionprovider.h"

#include "buffermodel.h"
#include "buffersettings.h"
#include "iconloader.h"
#include "clientidentity.h"
#include "network.h"
#include "util.h"

NetworkModelActionProvider::NetworkModelActionProvider(QObject *parent)
: AbstractActionProvider(parent),
  _actionCollection(new ActionCollection(this)),
  _messageFilter(0),
  _receiver(0)
{
  registerAction(NetworkConnect, SmallIcon("network-connect"), tr("Connect"));
  registerAction(NetworkDisconnect, SmallIcon("network-disconnect"), tr("Disconnect"));

  registerAction(BufferJoin, tr("Join"));
  registerAction(BufferPart, tr("Part"));
  registerAction(BufferRemove, tr("Delete Buffer(s)..."));
  registerAction(BufferSwitchTo, tr("Show Buffer"));

  registerAction(HideJoin, tr("Joins"), true);
  registerAction(HidePart, tr("Parts"), true);
  registerAction(HideQuit, tr("Quits"), true);
  registerAction(HideNick, tr("Nick Changes"), true);
  registerAction(HideMode, tr("Mode Changes"), true);
  registerAction(HideDayChange, tr("Day Changes"), true);
  registerAction(HideApplyToAll, tr("Set as Default..."));
  registerAction(HideUseDefaults, tr("Use Defaults..."));

  registerAction(JoinChannel, tr("Join Channel..."));

  registerAction(NickQuery, tr("Start Query"));
  registerAction(NickSwitchTo, tr("Show Query"));
  registerAction(NickWhois, tr("Whois"));

  registerAction(NickCtcpVersion, tr("Version"));
  registerAction(NickCtcpTime, tr("Time"));
  registerAction(NickCtcpPing, tr("Ping"));
  registerAction(NickCtcpFinger, tr("Finger"));

  registerAction(NickOp, tr("Give Operator Status"));
  registerAction(NickDeop, tr("Take Operator Status"));
  registerAction(NickVoice, tr("Give Voice"));
  registerAction(NickDevoice, tr("Take Voice"));
  registerAction(NickKick, tr("Kick From Channel"));
  registerAction(NickBan, tr("Ban From Channel"));
  registerAction(NickKickBan, tr("Kick && Ban"));

  registerAction(HideBufferTemporarily, tr("Hide Buffer(s) Temporarily"));
  registerAction(HideBufferPermanently, tr("Hide Buffer(s) Permanently"));
  registerAction(ShowChannelList, SmallIcon("format-list-unordered"), tr("Show Channel List"));
  registerAction(ShowIgnoreList, tr("Show Ignore List"));

  connect(_actionCollection, SIGNAL(actionTriggered(QAction *)), SLOT(actionTriggered(QAction *)));

  QMenu *hideEventsMenu = new QMenu();
  hideEventsMenu->addAction(action(HideJoin));
  hideEventsMenu->addAction(action(HidePart));
  hideEventsMenu->addAction(action(HideQuit));
  hideEventsMenu->addAction(action(HideNick));
  hideEventsMenu->addAction(action(HideMode));
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

NetworkModelActionProvider::~NetworkModelActionProvider() {
  _hideEventsMenuAction->menu()->deleteLater();
  _hideEventsMenuAction->deleteLater();
  _nickCtcpMenuAction->menu()->deleteLater();
  _nickCtcpMenuAction->deleteLater();
  _nickModeMenuAction->menu()->deleteLater();
  _nickModeMenuAction->deleteLater();
}

void NetworkModelActionProvider::registerAction(ActionType type, const QString &text, bool checkable) {
  registerAction(type, QPixmap(), text, checkable);
}

void NetworkModelActionProvider::registerAction(ActionType type, const QPixmap &icon, const QString &text, bool checkable) {
  Action *act;
  if(icon.isNull())
    act = new Action(text, this);
  else
    act = new Action(icon, text, this);

  act->setCheckable(checkable);
  act->setData(type);

  _actionCollection->addAction(QString::number(type, 16), act);
  _actionByType[type] = act;
}

void NetworkModelActionProvider::addActions(QMenu *menu, BufferId bufId, QObject *receiver, const char *method) {
  if(!bufId.isValid())
    return;
  addActions(menu, Client::networkModel()->bufferIndex(bufId), receiver, method);
}

void NetworkModelActionProvider::addActions(QMenu *menu, const QModelIndex &index, QObject *receiver, const char *method, bool isCustomBufferView) {
  if(!index.isValid())
    return;
  addActions(menu, QList<QModelIndex>() << index, 0, QString(), receiver, method, isCustomBufferView);
}

void NetworkModelActionProvider::addActions(QMenu *menu, MessageFilter *filter, BufferId msgBuffer, QObject *receiver, const char *slot) {
  addActions(menu, filter, msgBuffer, QString(), receiver, slot);
}

void NetworkModelActionProvider::addActions(QMenu *menu, MessageFilter *filter, BufferId msgBuffer, const QString &chanOrNick, QObject *receiver, const char *method) {
  if(!filter)
    return;
  addActions(menu, QList<QModelIndex>() << Client::networkModel()->bufferIndex(msgBuffer), filter, chanOrNick, receiver, method, false);
}

void NetworkModelActionProvider::addActions(QMenu *menu, const QList<QModelIndex> &indexList, QObject *receiver,  const char *method, bool isCustomBufferView) {
  addActions(menu, indexList, 0, QString(), receiver, method, isCustomBufferView);
}

// add a list of actions sensible for the current item(s)
void NetworkModelActionProvider::addActions(QMenu *menu,
                                            const QList<QModelIndex> &indexList,
                                            MessageFilter *filter,
                                            const QString &contextItem,
                                            QObject *receiver,
                                            const char *method,
                                            bool isCustomBufferView)
{
  if(!indexList.count())
    return;

  _indexList = indexList;
  _messageFilter = filter;
  _contextItem = contextItem;
  _receiver = receiver;
  _method = method;

  if(!_messageFilter) {
    // this means we are in a BufferView (or NickView) rather than a ChatView

    // first index in list determines the menu type (just in case we have both buffers and networks selected, for example)
    QModelIndex index = _indexList.at(0);
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
    if(_contextItem.isEmpty()) {
      // a) query buffer: handle like ircuser
      // b) general chatview: handle like channel iff it displays a single buffer
      // NOTE stuff breaks probably with merged buffers, need to rework a lot around here then
      if(_messageFilter->containedBuffers().count() == 1) {
        // we can handle this like a single bufferItem
        QModelIndex index = Client::networkModel()->bufferIndex(_messageFilter->containedBuffers().values().at(0));
        _indexList = QList<QModelIndex>() << index;
        addBufferItemActions(menu, index);
        return;
      } else {
        // TODO: actions for merged buffers... _indexList contains the index of the message we clicked on

      }
    } else {
      // context item = chan or nick, _indexList = buf where the msg clicked on originated
      if(isChannelName(_contextItem)) {
        QModelIndex msgIdx = _indexList.at(0);
        if(!msgIdx.isValid())
          return;
        NetworkId networkId = msgIdx.data(NetworkModel::NetworkIdRole).value<NetworkId>();
        BufferId bufId = Client::networkModel()->bufferId(networkId, _contextItem);
        if(bufId.isValid()) {
          QModelIndex targetIdx = Client::networkModel()->bufferIndex(bufId);
          _indexList = QList<QModelIndex>() << targetIdx;
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

void NetworkModelActionProvider::addNetworkItemActions(QMenu *menu, const QModelIndex &index) {
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

void NetworkModelActionProvider::addBufferItemActions(QMenu *menu, const QModelIndex &index, bool isCustomBufferView) {
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

void NetworkModelActionProvider::addIrcUserActions(QMenu *menu, const QModelIndex &index) {
  // this can be called: a) as a nicklist context menu (index has IrcUserItemType)
  //                     b) as a query buffer context menu (index has BufferItemType and is a QueryBufferItem)
  //                     c) right-click in a query chatview (same as b), index will be the corresponding QueryBufferItem)
  //                     d) right-click on some nickname (_contextItem will be non-null, _filter -> chatview, index -> message buffer)

  if(_contextItem.isNull()) {
    // cases a, b, c
    bool haveQuery = _indexList.count() == 1 && findQueryBuffer(index).isValid();
    NetworkModel::ItemType itemType = static_cast<NetworkModel::ItemType>(index.data(NetworkModel::ItemTypeRole).toInt());
    addAction(_nickModeMenuAction, menu, itemType == NetworkModel::IrcUserItemType);
    addAction(_nickCtcpMenuAction, menu);
    menu->addSeparator();
    addAction(NickQuery, menu, itemType == NetworkModel::IrcUserItemType && !haveQuery && _indexList.count() == 1);
    addAction(NickSwitchTo, menu, itemType == NetworkModel::IrcUserItemType && haveQuery);
    menu->addSeparator();
    addAction(NickWhois, menu, true);

  } else if(!_contextItem.isEmpty() && _messageFilter) {
    // case d
    // TODO

  }
}

/******** Helper Functions ***********************************************************************/

bool NetworkModelActionProvider::checkRequirements(const QModelIndex &index, ItemActiveStates requiredActiveState) {
  if(!index.isValid())
    return false;

  ItemActiveStates isActive = index.data(NetworkModel::ItemActiveRole).toBool()
  ? ActiveState
  : InactiveState;

  if(!(isActive & requiredActiveState))
    return false;

  return true;
}

Action * NetworkModelActionProvider::addAction(ActionType type , QMenu *menu, const QModelIndex &index, ItemActiveStates requiredActiveState) {
  return addAction(action(type), menu, checkRequirements(index, requiredActiveState));
}

Action * NetworkModelActionProvider::addAction(Action *action , QMenu *menu, const QModelIndex &index, ItemActiveStates requiredActiveState) {
  return addAction(action, menu, checkRequirements(index, requiredActiveState));
}

Action * NetworkModelActionProvider::addAction(ActionType type , QMenu *menu, bool condition) {
  return addAction(action(type), menu, condition);
}

Action * NetworkModelActionProvider::addAction(Action *action , QMenu *menu, bool condition) {
  if(condition) {
    menu->addAction(action);
    action->setVisible(true);
  } else {
    action->setVisible(false);
  }
  return action;
}

void NetworkModelActionProvider::addHideEventsMenu(QMenu *menu, BufferId bufferId) {
  if(BufferSettings(bufferId).hasFilter())
    addHideEventsMenu(menu, BufferSettings(bufferId).messageFilter());
  else
    addHideEventsMenu(menu);
}

void NetworkModelActionProvider::addHideEventsMenu(QMenu *menu, MessageFilter *msgFilter) {
  if(BufferSettings(msgFilter->idString()).hasFilter())
    addHideEventsMenu(menu, BufferSettings(msgFilter->idString()).messageFilter());
  else
    addHideEventsMenu(menu);
}

void NetworkModelActionProvider::addHideEventsMenu(QMenu *menu, int filter) {
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

  menu->addAction(_hideEventsMenuAction);
}

QString NetworkModelActionProvider::nickName(const QModelIndex &index) const {
  IrcUser *ircUser = qobject_cast<IrcUser *>(index.data(NetworkModel::IrcUserRole).value<QObject *>());
  if(ircUser)
    return ircUser->nick();

  BufferInfo bufferInfo = index.data(NetworkModel::BufferInfoRole).value<BufferInfo>();
  if(!bufferInfo.isValid())
    return QString();
  if(!bufferInfo.type() == BufferInfo::QueryBuffer)
    return QString();

  return bufferInfo.bufferName(); // FIXME this might break with merged queries maybe
}

BufferId NetworkModelActionProvider::findQueryBuffer(const QModelIndex &index, const QString &predefinedNick) const {
  NetworkId networkId = index.data(NetworkModel::NetworkIdRole).value<NetworkId>();
  if(!networkId.isValid())
    return BufferId();

  QString nick = predefinedNick.isEmpty() ? nickName(index) : predefinedNick;
  if(nick.isEmpty())
    return BufferId();

  return findQueryBuffer(networkId, nick);
}

BufferId NetworkModelActionProvider::findQueryBuffer(NetworkId networkId, const QString &nick) const {
  return Client::networkModel()->bufferId(networkId, nick);
}

void NetworkModelActionProvider::handleExternalAction(ActionType type, QAction *action) {
  Q_UNUSED(type);
  if(_receiver && _method) {
    if(!QMetaObject::invokeMethod(_receiver, _method, Q_ARG(QAction *, action)))
      qWarning() << "NetworkModelActionProvider::handleExternalAction(): Could not invoke slot" << _receiver << _method;
  }
}

/******** Handle Actions *************************************************************************/

void NetworkModelActionProvider::actionTriggered(QAction *action) {
  ActionType type = (ActionType)action->data().toInt();
  if(type > 0) {
    if(type & NetworkMask)
      handleNetworkAction(type, action);
    else if(type & BufferMask)
      handleBufferAction(type, action);
    else if(type & HideMask)
      handleHideAction(type, action);
    else if(type & GeneralMask)
      handleGeneralAction(type, action);
    else if(type & NickMask)
      handleNickAction(type, action);
    else if(type & ExternalMask)
      handleExternalAction(type, action);
    else
      qWarning() << "NetworkModelActionProvider::actionTriggered(): Unhandled action!";
  }
  _indexList.clear();
  _messageFilter = 0;
  _receiver = 0;
}

void NetworkModelActionProvider::handleNetworkAction(ActionType type, QAction *) {
  if(!_indexList.count())
    return;
  const Network *network = Client::network(_indexList.at(0).data(NetworkModel::NetworkIdRole).value<NetworkId>());
  Q_CHECK_PTR(network);
  if(!network)
    return;

  switch(type) {
    case NetworkConnect:
      network->requestConnect();
      break;
    case NetworkDisconnect:
      network->requestDisconnect();
      break;
    default:
      break;
  }
}

void NetworkModelActionProvider::handleBufferAction(ActionType type, QAction *) {
  if(type == BufferRemove) {
    removeBuffers(_indexList);
  } else {

    foreach(QModelIndex index, _indexList) {
      BufferInfo bufferInfo = index.data(NetworkModel::BufferInfoRole).value<BufferInfo>();
      if(!bufferInfo.isValid())
        continue;

      switch(type) {
        case BufferJoin:
          Client::userInput(bufferInfo, QString("/JOIN %1").arg(bufferInfo.bufferName()));
          break;
        case BufferPart:
        {
          QString reason = Client::identity(Client::network(bufferInfo.networkId())->identity())->partReason();
          Client::userInput(bufferInfo, QString("/PART %1").arg(reason));
          break;
        }
        case BufferSwitchTo:
          Client::bufferModel()->switchToBuffer(bufferInfo.bufferId());
          break;
        default:
          break;
      }
    }
  }
}

void NetworkModelActionProvider::removeBuffers(const QModelIndexList &indexList) {
  QList<BufferInfo> inactive;
  foreach(QModelIndex index, indexList) {
    BufferInfo info = index.data(NetworkModel::BufferInfoRole).value<BufferInfo>();
    if(info.isValid()) {
      if(info.type() == BufferInfo::QueryBuffer
        || (info.type() == BufferInfo::ChannelBuffer && !index.data(NetworkModel::ItemActiveRole).toBool()))
          inactive << info;
    }
  }
  QString msg;
  if(inactive.count()) {
    msg = tr("Do you want to delete the following buffer(s) permanently?", 0, inactive.count());
    msg += "<ul>";
    foreach(BufferInfo info, inactive)
      msg += QString("<li>%1</li>").arg(info.bufferName());
    msg += "</ul>";
    msg += tr("<b>Note:</b> This will delete all related data, including all backlog data, from the core's database and cannot be undone.");
    if(inactive.count() != indexList.count())
      msg += tr("<br>Active channel buffers cannot be deleted, please part the channel first.");

    if(QMessageBox::question(0, tr("Remove buffers permanently?"), msg, QMessageBox::Yes|QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
      foreach(BufferInfo info, inactive)
        Client::removeBuffer(info.bufferId());
    }
  }
}

void NetworkModelActionProvider::handleHideAction(ActionType type, QAction *action) {
  Q_UNUSED(action)

  int filter = 0;
  if(NetworkModelActionProvider::action(HideJoin)->isChecked())
    filter |= Message::Join;
  if(NetworkModelActionProvider::action(HidePart)->isChecked())
    filter |= Message::Part;
  if(NetworkModelActionProvider::action(HideQuit)->isChecked())
    filter |= Message::Quit;
  if(NetworkModelActionProvider::action(HideNick)->isChecked())
    filter |= Message::Nick;
  if(NetworkModelActionProvider::action(HideMode)->isChecked())
    filter |= Message::Mode;
  if(NetworkModelActionProvider::action(HideDayChange)->isChecked())
    filter |= Message::DayChange;

  switch(type) {
  case HideJoin:
  case HidePart:
  case HideQuit:
  case HideNick:
  case HideMode:
  case HideDayChange:
    if(_messageFilter)
      BufferSettings(_messageFilter->idString()).setMessageFilter(filter);
    else {
      foreach(QModelIndex index, _indexList) {
	BufferId bufferId = index.data(NetworkModel::BufferIdRole).value<BufferId>();
	if(!bufferId.isValid())
	  continue;
	BufferSettings(bufferId).setMessageFilter(filter);
      }
    }
    return;
  case HideApplyToAll:
    BufferSettings().setMessageFilter(filter);
  case HideUseDefaults:
    if(_messageFilter)
      BufferSettings(_messageFilter->idString()).removeFilter();
    else {
      foreach(QModelIndex index, _indexList) {
	BufferId bufferId = index.data(NetworkModel::BufferIdRole).value<BufferId>();
	if(!bufferId.isValid())
	  continue;
	BufferSettings(bufferId).removeFilter();
      }
    }
    return;
  default:
    return;
  };
}

void NetworkModelActionProvider::handleGeneralAction(ActionType type, QAction *action) {
  Q_UNUSED(action)

  if(!_indexList.count())
    return;
  NetworkId networkId = _indexList.at(0).data(NetworkModel::NetworkIdRole).value<NetworkId>();
  if(!networkId.isValid())
    return;

  switch(type) {
    case JoinChannel: {
      QString channelName = _contextItem;
      if(channelName.isEmpty()) {
        bool ok;
        channelName = QInputDialog::getText(0, tr("Join Channel"), tr("Input channel name:"), QLineEdit::Normal, QString(), &ok);
        if(!ok)
          return;
      }
      if(!channelName.isEmpty()) {
        Client::instance()->userInput(BufferInfo::fakeStatusBuffer(networkId), QString("/JOIN %1").arg(channelName));
      }
      break;
    }
    case ShowChannelList:
      emit showChannelList(networkId);
      break;
    case ShowIgnoreList:
      emit showIgnoreList(networkId);
      break;
    default:
      break;
  }
}

void NetworkModelActionProvider::handleNickAction(ActionType type, QAction *) {
  foreach(QModelIndex index, _indexList) {
    NetworkId networkId = index.data(NetworkModel::NetworkIdRole).value<NetworkId>();
    if(!networkId.isValid())
      continue;
    QString nick = nickName(index);
    if(nick.isEmpty())
      continue;
    BufferInfo bufferInfo = index.data(NetworkModel::BufferInfoRole).value<BufferInfo>();
    if(!bufferInfo.isValid())
      continue;

    switch(type) {
      case NickWhois:
        Client::userInput(bufferInfo, QString("/WHOIS %1 %1").arg(nick));
        break;
      case NickCtcpVersion:
        Client::userInput(bufferInfo, QString("/CTCP %1 VERSION").arg(nick));
        break;
      case NickCtcpPing:
        Client::userInput(bufferInfo, QString("/CTCP %1 PING").arg(nick));
        break;
      case NickCtcpTime:
        Client::userInput(bufferInfo, QString("/CTCP %1 TIME").arg(nick));
        break;
      case NickCtcpFinger:
        Client::userInput(bufferInfo, QString("/CTCP %1 FINGER").arg(nick));
        break;
      case NickOp:
        Client::userInput(bufferInfo, QString("/OP %1").arg(nick));
        break;
      case NickDeop:
        Client::userInput(bufferInfo, QString("/DEOP %1").arg(nick));
        break;
      case NickVoice:
        Client::userInput(bufferInfo, QString("/VOICE %1").arg(nick));
        break;
      case NickDevoice:
        Client::userInput(bufferInfo, QString("/DEVOICE %1").arg(nick));
        break;
      case NickKick:
        Client::userInput(bufferInfo, QString("/KICK %1").arg(nick));
        break;
      case NickBan:
        Client::userInput(bufferInfo, QString("/BAN %1").arg(nick));
        break;
      case NickKickBan:
        Client::userInput(bufferInfo, QString("/BAN %1").arg(nick));
        Client::userInput(bufferInfo, QString("/KICK %1").arg(nick));
        break;
      case NickSwitchTo:
        Client::bufferModel()->switchToBuffer(findQueryBuffer(networkId, nick));
        break;
      case NickQuery:
        Client::userInput(bufferInfo, QString("/QUERY %1").arg(nick));
        break;
      default:
        qWarning() << "Unhandled nick action";
    }
  }
}
