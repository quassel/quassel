/***************************************************************************
 *   Copyright (C) 2005-2013 by the Quassel Project                        *
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

#ifndef CONTEXTMENUACTIONPROVIDER_H
#define CONTEXTMENUACTIONPROVIDER_H

#include "networkmodelcontroller.h"

class ContextMenuActionProvider : public NetworkModelController
{
    Q_OBJECT

public:
    ContextMenuActionProvider(QObject *parent = 0);
    virtual ~ContextMenuActionProvider();

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

private:

    void addActions(QMenu *, const QList<QModelIndex> &indexList, MessageFilter *filter, const QString &chanOrNick,
        QObject *receiver, const char *slot, bool allowBufferHide);

    Action *addAction(ActionType, QMenu *, bool condition = true);
    Action *addAction(Action *, QMenu *, bool condition = true);
    Action *addAction(ActionType, QMenu *, const QModelIndex &index, ItemActiveStates requiredActiveState = QFlags<ItemActiveState>(ActiveState | InactiveState));
    Action *addAction(Action *, QMenu *, const QModelIndex &index, ItemActiveStates requiredActiveState = QFlags<ItemActiveState>(ActiveState | InactiveState));

    void addHideEventsMenu(QMenu *, BufferId bufferId);
    void addHideEventsMenu(QMenu *, MessageFilter *msgFilter);
    void addHideEventsMenu(QMenu *, int filter = -1);
    void addIgnoreMenu(QMenu *menu, const QString &hostmask, const QMap<QString, bool> &ignoreMap);

    void addNetworkItemActions(QMenu *, const QModelIndex &);
    void addBufferItemActions(QMenu *, const QModelIndex &, bool isCustomBufferView = false);
    void addIrcUserActions(QMenu *, const QModelIndex &);

    Action *_hideEventsMenuAction;
    Action *_nickCtcpMenuAction;
    Action *_nickModeMenuAction;
    Action *_nickIgnoreMenuAction;
    QList<QAction *> _ignoreDescriptions;
};


#endif
