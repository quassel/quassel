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

#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>

#include "networkmodelactionprovider.h"

#include "buffersettings.h"
#include "iconloader.h"
#include "identity.h"
#include "network.h"

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
  registerAction(BufferRemove, tr("Delete Buffer..."));

  registerAction(HideJoin, tr("Joins"), true);
  registerAction(HidePart, tr("Parts"), true);
  registerAction(HideQuit, tr("Quits"), true);
  registerAction(HideNick, tr("Nick Changes"), true);
  registerAction(HideMode, tr("Mode Changes"), true);
  registerAction(HideDayChange, tr("Day Changes"), true);
  registerAction(HideApplyToAll, tr("Apply to All Chat Views..."));

  registerAction(JoinChannel, tr("Join Channel..."));

  registerAction(NickCtcpWhois, tr("Whois"));
  registerAction(NickCtcpVersion, tr("Version"));
  registerAction(NickCtcpTime, tr("Time"));
  registerAction(NickCtcpPing, tr("Ping"));
  registerAction(NickCtcpFinger, tr("Finger"));

  registerAction(NickOp, tr("Give Operator Status"));
  registerAction(NickDeop, tr("Take Operator Status"));
  registerAction(NickVoice, tr("Give Voice"));
  registerAction(NickDevoice, tr("Take Voice"));
  registerAction(NickKick, tr("Kick"));
  registerAction(NickBan, tr("Ban"));
  registerAction(NickKickBan, tr("Kickban"));
  registerAction(NickQuery, tr("Query"));

  registerAction(HideBufferTemporarily, tr("Hide Buffer(s) Temporarily"));
  registerAction(HideBufferPermanently, tr("Hide Buffer(s) Permanently"));
  registerAction(ShowChannelList, SmallIcon("format-list-unordered"), tr("Show Channel List"));
  registerAction(ShowIgnoreList, tr("Show Ignore List"));

  connect(_actionCollection, SIGNAL(actionTriggered(QAction *)), SLOT(actionTriggered(QAction *)));

  action(HideApplyToAll)->setDisabled(true);

  QMenu *hideEventsMenu = new QMenu();
  hideEventsMenu->addAction(action(HideJoin));
  hideEventsMenu->addAction(action(HidePart));
  hideEventsMenu->addAction(action(HideQuit));
  hideEventsMenu->addAction(action(HideNick));
  hideEventsMenu->addAction(action(HideMode));
  hideEventsMenu->addAction(action(HideDayChange));
  hideEventsMenu->addSeparator();
  hideEventsMenu->addAction(action(HideApplyToAll));
  _hideEventsMenuAction = new Action(tr("Hide Events"), 0);
  _hideEventsMenuAction->setMenu(hideEventsMenu);
}

NetworkModelActionProvider::~NetworkModelActionProvider() {
  _hideEventsMenuAction->menu()->deleteLater();
  _hideEventsMenuAction->deleteLater();
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
  _messageFilter = 0;
  addActions(menu, Client::networkModel()->bufferIndex(bufId), receiver, method);
}

void NetworkModelActionProvider::addActions(QMenu *menu, MessageFilter *filter, QObject *receiver, const char *method) {
  if(!filter)
    return;
  _messageFilter = filter;
  QList<QModelIndex> indexes;
  foreach(BufferId bufId, filter->containedBuffers())
    indexes << Client::networkModel()->bufferIndex(bufId);
  addActions(menu, indexes, receiver, method);
}

void NetworkModelActionProvider::addActions(QMenu *menu, const QModelIndex &index, QObject *receiver, const char *method, bool isCustomBufferView) {
  if(!index.isValid())
    return;
  _messageFilter = 0;
  addActions(menu, QList<QModelIndex>() << index, receiver, method, isCustomBufferView);
}

// add a list of actions sensible for the current item(s)
void NetworkModelActionProvider::addActions(QMenu *menu,
                                            const QList<QModelIndex> &indexList,
                                            QObject *receiver,
                                            const char *method,
                                            bool isCustomBufferView)
{
  if(!indexList.count())
    return;

  _indexList = indexList;
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

      default:
        return;

    }
  } else {
    // ChatView actions


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

  switch(bufferInfo.type()) {
    case BufferInfo::ChannelBuffer:
      addAction(BufferJoin, menu, index, InactiveState);
      addAction(BufferPart, menu, index, ActiveState);
      menu->addSeparator();
      addHideEventsMenu(menu, bufferInfo.bufferId());
      menu->addSeparator();
      addAction(HideBufferTemporarily, menu, isCustomBufferView);
      addAction(HideBufferPermanently, menu, isCustomBufferView);
      addAction(BufferRemove, menu, index);
      break;

    case BufferInfo::QueryBuffer:
    {
      IrcUser *ircUser = qobject_cast<IrcUser *>(index.data(NetworkModel::IrcUserRole).value<QObject *>());
      if(ircUser) {
        addIrcUserActions(menu, ircUser, false);
        menu->addSeparator();
      }
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

void NetworkModelActionProvider::addIrcUserActions(QMenu *menu, IrcUser *ircUser, bool isNickView) {


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
  addHideEventsMenu(menu, BufferSettings(bufferId).messageFilter());
}

void NetworkModelActionProvider::addHideEventsMenu(QMenu *menu, MessageFilter *msgFilter) {
  addHideEventsMenu(menu, BufferSettings(msgFilter->idString()).messageFilter());
}

void NetworkModelActionProvider::addHideEventsMenu(QMenu *menu, int filter) {
  action(HideJoin)->setChecked(filter & Message::Join);
  action(HidePart)->setChecked(filter & Message::Part);
  action(HideQuit)->setChecked(filter & Message::Quit);
  action(HideNick)->setChecked(filter & Message::Nick);
  action(HideMode)->setChecked(filter & Message::Mode);
  action(HideDayChange)->setChecked(filter & Message::DayChange);

  menu->addAction(_hideEventsMenuAction);
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
  foreach(QModelIndex index, _indexList) {
    BufferInfo bufferInfo = index.data(NetworkModel::BufferInfoRole).value<BufferInfo>();
    if(!bufferInfo.isValid())
      continue;

    switch(type) {
      case BufferJoin:
        Client::instance()->userInput(bufferInfo, QString("/JOIN %1").arg(bufferInfo.bufferName()));
        break;
      case BufferPart:
      {
        QString reason = Client::identity(Client::network(bufferInfo.networkId())->identity())->partReason();
        Client::instance()->userInput(bufferInfo, QString("/PART %1").arg(reason));
        break;
      }
      case BufferRemove:
      {
        int res = QMessageBox::question(0, tr("Remove buffer permanently?"),
        tr("Do you want to delete the buffer \"%1\" permanently? This will delete all related data, including all backlog "
        "data, from the core's database!").arg(bufferInfo.bufferName()),
        QMessageBox::Yes|QMessageBox::No, QMessageBox::No);
        if(res == QMessageBox::Yes) {
          Client::removeBuffer(bufferInfo.bufferId());
        }
        break;
      }
      default:
        break;
    }
  }
}

void NetworkModelActionProvider::handleHideAction(ActionType type, QAction *action) {
  Message::Type msgType;
  switch(type) {
    case HideJoin:
      msgType = Message::Join; break;
    case HidePart:
      msgType = Message::Part; break;
    case HideQuit:
      msgType = Message::Quit; break;
    case HideNick:
      msgType = Message::Nick; break;
    case HideMode:
      msgType = Message::Mode; break;
    case HideDayChange:
      msgType = Message::DayChange; break;
    case HideApplyToAll:
      // TODO implement "apply to all" for hiding messages
      break;
    default:
      return;
  }

  if(_messageFilter)
    BufferSettings(_messageFilter->idString()).filterMessage(msgType, action->isChecked());
  else {
    foreach(QModelIndex index, _indexList) {
      BufferId bufferId = index.data(NetworkModel::BufferIdRole).value<BufferId>();
      if(!bufferId.isValid())
        continue;
      BufferSettings(bufferId).filterMessage(msgType, action->isChecked());
    }
  }
}

void NetworkModelActionProvider::handleGeneralAction(ActionType type, QAction *action) {
  if(!_indexList.count())
    return;
  NetworkId networkId = _indexList.at(0).data(NetworkModel::NetworkIdRole).value<NetworkId>();
  if(!networkId.isValid())
    return;

  switch(type) {
    case JoinChannel:
    {
    //  FIXME no QInputDialog in Qtopia
#   ifndef Q_WS_QWS
      bool ok;
      QString channelName = QInputDialog::getText(0, tr("Join Channel"), tr("Input channel name:"), QLineEdit::Normal, QString(), &ok);
      if(ok && !channelName.isEmpty()) {
        Client::instance()->userInput(BufferInfo::fakeStatusBuffer(networkId),
                                      QString("/JOIN %1").arg(channelName));
      }
#   endif
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


}

void NetworkModelActionProvider::handleExternalAction(ActionType type, QAction *action) {
  Q_UNUSED(type);
  if(_receiver && _method) {
    if(!QMetaObject::invokeMethod(_receiver, _method, Q_ARG(QAction *, action)))
      qWarning() << "NetworkModelActionProvider::handleExternalAction(): Could not invoke slot" << _receiver << _method;
  }
}
