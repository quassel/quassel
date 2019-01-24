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

#pragma once

#include "uisupport-export.h"

#include <functional>

#include "networkmodelcontroller.h"

class UISUPPORT_EXPORT ContextMenuActionProvider : public NetworkModelController
{
    Q_OBJECT

public:
    ContextMenuActionProvider(QObject* parent = nullptr);
    ~ContextMenuActionProvider() override;

    //! Provide a list of actions applying to the given item
    /**
     * Note that this list ist not persistent, hence it should only be used immediately after
     * calling this function (e.g. in a context menu). Optionally one can provide a QObject and a slot
     * (that should take a QAction * as parameter) that is invoked for actions that require external
     * handling.
     * @param index The item index in the NetworkModel
     * @param receiver The optional object that is notified for actions that need to be handled externally.
     *                 The action type will be stored in the QAction's data().
     * @param slot     The receiving slot
     * @return A list of actions applying to the given item
     */
    template<typename Receiver, typename Slot>
    void addActions(QMenu* menu, const QModelIndex& index, Receiver* receiver, Slot slot, bool isCustomBufferView = false)
    {
        addActions(menu, index, buildActionSlot(receiver, std::move(slot)), isCustomBufferView);
    }
    void addActions(QMenu* menu, const QModelIndex& index, ActionSlot = {}, bool isCustomBufferView = false);

    template<typename Receiver, typename Slot>
    void addActions(QMenu* menu, const QList<QModelIndex>& indexList, Receiver* receiver, Slot slot, bool isCustomBufferView = false)
    {
        addActions(menu, indexList, buildActionSlot(receiver, std::move(slot)), isCustomBufferView);
    }
    void addActions(QMenu* menu, const QList<QModelIndex>& indexList, ActionSlot = {}, bool isCustomBufferView = false);

    template<typename Receiver, typename Slot>
    void addActions(QMenu* menu, BufferId id, Receiver* receiver, Slot slot)
    {
        addActions(menu, id, buildActionSlot(receiver, std::move(slot)));
    }
    void addActions(QMenu* menu, BufferId id, ActionSlot = {});

    template<typename Receiver, typename Slot>
    void addActions(QMenu* menu, MessageFilter* filter, BufferId msgBuffer, Receiver* receiver, Slot slot)
    {
        addActions(menu, filter, msgBuffer, buildActionSlot(receiver, std::move(slot)));
    }
    void addActions(QMenu* menu, MessageFilter* filter, BufferId msgBuffer, ActionSlot = {});

    template<typename Receiver, typename Slot>
    void addActions(QMenu* menu, MessageFilter* filter, BufferId msgBuffer, const QString& chanOrNick, Receiver* receiver, Slot slot)
    {
        addActions(menu, filter, msgBuffer, chanOrNick, buildActionSlot(receiver, std::move(slot)));
    }
    void addActions(QMenu* menu, MessageFilter* filter, BufferId msgBuffer, const QString& chanOrNick, ActionSlot = {});

private:
    void addActions(QMenu* menu,
                    const QList<QModelIndex>& indexList,
                    MessageFilter* filter,
                    const QString& chanOrNick,
                    ActionSlot actionSlot,
                    bool isCustomBufferView);

    Action* addAction(ActionType, QMenu*, bool condition = true);
    Action* addAction(Action*, QMenu*, bool condition = true);
    Action* addAction(ActionType,
                      QMenu*,
                      const QModelIndex& index,
                      ItemActiveStates requiredActiveState = QFlags<ItemActiveState>(ActiveState | InactiveState));
    Action* addAction(Action*,
                      QMenu*,
                      const QModelIndex& index,
                      ItemActiveStates requiredActiveState = QFlags<ItemActiveState>(ActiveState | InactiveState));

    void addHideEventsMenu(QMenu*, BufferId bufferId);
    void addHideEventsMenu(QMenu*, MessageFilter* msgFilter);
    void addHideEventsMenu(QMenu*, int filter = -1);
    void addIgnoreMenu(QMenu* menu, const QString& hostmask, const QMap<QString, bool>& ignoreMap);

    void addNetworkItemActions(QMenu*, const QModelIndex&);
    void addBufferItemActions(QMenu*, const QModelIndex&, bool isCustomBufferView = false);
    void addIrcUserActions(QMenu*, const QModelIndex&);

    Action* _hideEventsMenuAction;
    Action* _nickCtcpMenuAction;
    Action* _nickModeMenuAction;
    Action* _nickIgnoreMenuAction;
    QList<QAction*> _ignoreDescriptions;
};
