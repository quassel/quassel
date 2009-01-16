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

#ifndef NETWORKMODELACTIONPROVIDER_H
#define NETWORKMODELACTIONPROVIDER_H

#include "action.h"
#include "actioncollection.h"
#include "messagefilter.h"
#include "quasselui.h"

class NetworkModelActionProvider : public AbstractActionProvider {
  Q_OBJECT

public:
  NetworkModelActionProvider(QObject *parent = 0);
  ~NetworkModelActionProvider();

  // don't change enums without doublechecking masks etc. in code
  enum ActionType {
    // Network actions
    NetworkMask = 0x0f,
    NetworkConnect = 0x01,
    NetworkDisconnect = 0x02,

    // Buffer actions
    BufferMask = 0xf0,
    BufferJoin = 0x10,
    BufferPart = 0x20,
    BufferSwitchTo = 0x30,
    BufferRemove = 0x40,

    // Hide actions
    HideMask = 0x0f00,
    HideJoin = 0x0100,
    HidePart = 0x0200,
    HideQuit = 0x0300,
    HideNick = 0x0400,
    HideMode = 0x0500,
    HideDayChange = 0x0600,
    HideUseDefaults = 0xe00,
    HideApplyToAll = 0xf00,

    // General actions
    GeneralMask = 0xf000,
    JoinChannel = 0x1000,
    ShowChannelList = 0x2000,
    ShowIgnoreList = 0x3000,

    // Nick actions
    NickMask = 0xff0000,
    NickWhois = 0x010000,
    NickQuery = 0x020000,
    NickSwitchTo = 0x030000,
    NickCtcpVersion = 0x040000,
    NickCtcpPing = 0x050000,
    NickCtcpTime = 0x060000,
    NickCtcpFinger = 0x070000,
    NickOp = 0x080000,
    NickDeop = 0x090000,
    NickVoice = 0x0a0000,
    NickDevoice = 0x0b0000,
    NickKick = 0x0c0000,
    NickBan = 0x0d0000,
    NickKickBan = 0x0e0000,

    // Actions that are handled externally
    // These emit a signal to the action requester, rather than being handled here
    ExternalMask = 0xff000000,
    HideBufferTemporarily = 0x01000000,
    HideBufferPermanently = 0x02000000
  };

  inline Action *action(ActionType type) const;

  //! Provide a list of actions applying to the given item
  /**
   * Note that this list ist not persistent, hence it should only be used immediately after
   * calling this function (e.g. in a context menu). Optionally one can provide a QObject and a slot
   * (that should take a QAction * as parameter) that is invoked for actions that require external
   * handling.
   * @param index The item index in the NetworkModel
   * @param receiver The optional object that is notified for actions that need to be handled externally.
   *                 The action type will be stored in the QAction's data().
   * @param slot     The receiving slot name; should be "mySlot" rather than SLOT(mySlot(QAction *))
   * @return A list of actions applying to the given item
   */
  void addActions(QMenu *, const QModelIndex &index, QObject *receiver = 0, const char *slot = 0, bool allowBufferHide = false);
  void addActions(QMenu *, const QList<QModelIndex> &indexList, QObject *receiver = 0, const char *slot = 0, bool allowBufferHide = false);
  void addActions(QMenu *, BufferId id, QObject *receiver = 0, const char *slot = 0);
  void addActions(QMenu *, MessageFilter *filter, BufferId msgBuffer, QObject *receiver = 0, const char *slot = 0);
  void addActions(QMenu *, MessageFilter *filter, BufferId msgBuffer, const QString &chanOrNick, QObject *receiver = 0, const char *slot = 0);

signals:
  void showChannelList(NetworkId);
  void showIgnoreList(NetworkId);

protected:
  inline ActionCollection *actionCollection() const;

protected slots:
  void actionTriggered(QAction *);

private:
  enum ItemActiveState {
    InactiveState = 0x01,
    ActiveState = 0x02
  };

public:
  Q_DECLARE_FLAGS(ItemActiveStates, ItemActiveState)

private:
  void registerAction(ActionType type, const QString &text, bool checkable = false);
  void registerAction(ActionType type, const QPixmap &icon, const QString &text, bool checkable = false);

  void handleNetworkAction(ActionType, QAction *);
  void handleBufferAction(ActionType, QAction *);
  void handleHideAction(ActionType, QAction *);
  void handleNickAction(ActionType, QAction *);
  void handleGeneralAction(ActionType, QAction *);
  void handleExternalAction(ActionType, QAction *);

  void addActions(QMenu *, const QList<QModelIndex> &indexList, MessageFilter *filter, const QString &chanOrNick,
                  QObject *receiver, const char *slot, bool allowBufferHide);

  bool checkRequirements(const QModelIndex &index, ItemActiveStates requiredActiveState = QFlags<ItemActiveState>(ActiveState | InactiveState));
  Action * addAction(ActionType, QMenu *, bool condition = true);
  Action * addAction(Action * , QMenu *, bool condition = true);
  Action * addAction(ActionType, QMenu *, const QModelIndex &index, ItemActiveStates requiredActiveState = QFlags<ItemActiveState>(ActiveState | InactiveState));
  Action * addAction(Action * , QMenu *, const QModelIndex &index, ItemActiveStates requiredActiveState = QFlags<ItemActiveState>(ActiveState | InactiveState));

  void addHideEventsMenu(QMenu *, BufferId bufferId);
  void addHideEventsMenu(QMenu *, MessageFilter *msgFilter);
  void addHideEventsMenu(QMenu *, int filter = -1);

  void addNetworkItemActions(QMenu *, const QModelIndex &);
  void addBufferItemActions(QMenu *, const QModelIndex &, bool isCustomBufferView = false);
  void addIrcUserActions(QMenu *, const QModelIndex &);

  QString nickName(const QModelIndex &index) const;
  BufferId findQueryBuffer(const QModelIndex &index, const QString &predefinedNick = QString()) const;
  BufferId findQueryBuffer(NetworkId, const QString &nickName) const;
  void removeBuffers(const QModelIndexList &indexList);

  NetworkModel *_model;

  ActionCollection *_actionCollection;
  QHash<ActionType, Action *> _actionByType;

  Action *_hideEventsMenuAction;
  Action *_nickCtcpMenuAction;
  Action *_nickModeMenuAction;

  QList<QModelIndex> _indexList;
  MessageFilter *_messageFilter;
  QString _contextItem;   ///< Channel name or nick to provide context menu for
  QObject *_receiver;
  const char *_method;
};

// inlines
ActionCollection *NetworkModelActionProvider::actionCollection() const { return _actionCollection; }
Action *NetworkModelActionProvider::action(ActionType type) const { return _actionByType.value(type, 0); }
#endif
