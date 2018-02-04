/***************************************************************************
 *   Copyright (C) 2005-2016 by the Quassel Project                        *
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

#include "chatmonitorview.h"

#include <QAction>
#include <QIcon>
#include <QMenu>
#include <QContextMenuEvent>

#include "chatmonitorfilter.h"
#include "chatlinemodel.h"
#include "chatitem.h"
#include "chatscene.h"
#include "client.h"
#include "networkmodel.h"
#include "buffermodel.h"
#include "messagemodel.h"
#include "qtuisettings.h"
#include "settingspagedlg.h"
#include "settingspages/chatmonitorsettingspage.h"
#include "clientignorelistmanager.h"

ChatMonitorView::ChatMonitorView(ChatMonitorFilter *filter, QWidget *parent)
    : ChatView(filter, parent),
    _filter(filter)
{
    scene()->setSenderCutoffMode(ChatScene::CutoffLeft);
    // The normal message prefixes get replaced by the network and buffer name.  Re-add brackets for
    // all message types.
    scene()->setAlwaysBracketSender(true);
    connect(Client::instance(), SIGNAL(coreConnectionStateChanged(bool)), this, SLOT(coreConnectionStateChanged(bool)));
}


void ChatMonitorView::addActionsToMenu(QMenu *menu, const QPointF &pos)
{
    ChatView::addActionsToMenu(menu, pos);
    menu->addSeparator();
    QAction *showOwnNicksAction = menu->addAction(tr("Show Own Messages"), _filter, SLOT(setShowOwnMessages(bool)));
    showOwnNicksAction->setCheckable(true);
    showOwnNicksAction->setChecked(_filter->showOwnMessages());

    if (scene()->columnByScenePos(pos) == ChatLineModel::SenderColumn) {
        menu->addSeparator();

        QAction *showNetworkAction = menu->addAction(tr("Show Network Name"), this, SLOT(showFieldsChanged(bool)));
        showNetworkAction->setCheckable(true);
        showNetworkAction->setChecked(_filter->showFields() & ChatMonitorFilter::NetworkField);
        showNetworkAction->setData(ChatMonitorFilter::NetworkField);

        QAction *showBufferAction = menu->addAction(tr("Show Buffer Name"), this, SLOT(showFieldsChanged(bool)));
        showBufferAction->setCheckable(true);
        showBufferAction->setChecked(_filter->showFields() & ChatMonitorFilter::BufferField);
        showBufferAction->setData(ChatMonitorFilter::BufferField);
    }

    menu->addSeparator();
    menu->addAction(QIcon::fromTheme("configure"), tr("Configure..."), this, SLOT(showSettingsPage()));
}


void ChatMonitorView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (scene()->columnByScenePos(event->pos()) != ChatLineModel::SenderColumn) {
        ChatView::mouseDoubleClickEvent(event);
        return;
    }

    ChatItem *chatItem = scene()->chatItemAt(mapToScene(event->pos()));
    if (!chatItem) {
        event->ignore();
        return;
    }

    event->accept();
    BufferId bufferId = chatItem->data(MessageModel::BufferIdRole).value<BufferId>();
    if (!bufferId.isValid())
        return;

    Client::bufferModel()->switchToBuffer(bufferId);
}


void ChatMonitorView::showFieldsChanged(bool checked)
{
    QAction *showAction = qobject_cast<QAction *>(sender());
    if (!showAction)
        return;

    if (checked)
        _filter->addShowField(showAction->data().toInt());
    else
        _filter->removeShowField(showAction->data().toInt());
}


void ChatMonitorView::showSettingsPage()
{
    SettingsPageDlg dlg(new ChatMonitorSettingsPage(), this);
    dlg.exec();
}


// connect only after client is synced to core since ChatMonitorView is created before
// the ignoreListManager
void ChatMonitorView::coreConnectionStateChanged(bool connected)
{
    if (connected)
        connect(Client::ignoreListManager(), SIGNAL(ignoreListChanged()), _filter, SLOT(invalidateFilter()));
}
