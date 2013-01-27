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

#ifndef TOOLBARACTIONPROVIDER_H_
#define TOOLBARACTIONPROVIDER_H_

#include "networkmodelcontroller.h"

class QToolBar;

class ToolBarActionProvider : public NetworkModelController
{
    Q_OBJECT

public:
    ToolBarActionProvider(QObject *parent = 0);
    virtual ~ToolBarActionProvider();

    enum ToolBarType {
        MainToolBar,
        ChatViewToolBar,
        NickToolBar
    };

    void addActions(QToolBar *, ToolBarType type);

public slots:
    virtual void disconnectedFromCore();

protected:
    virtual void handleNetworkAction(ActionType, QAction *);
    virtual void handleBufferAction(ActionType, QAction *);
    virtual void handleNickAction(ActionType, QAction *);
    virtual void handleGeneralAction(ActionType, QAction *);

private slots:
    void networkCreated(NetworkId id);
    void networkRemoved(NetworkId id);
    void networkUpdated(const Network *net = 0);
    void connectOrDisconnectNet();

    void currentBufferChanged(const QModelIndex &);
    void nickSelectionChanged(const QModelIndexList &);

    void updateStates();

private:
    QMenu *_networksConnectMenu, *_networksDisconnectMenu;
    QHash<NetworkId, Action *> _networkActions;
    QModelIndex _currentBuffer;
    QModelIndexList _selectedNicks;
};


#endif
