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

#include "awaylogview.h"

#include <QAction>
#include <QMenu>

#include "awaylogfilter.h"
#include "chatlinemodel.h"
#include "chatscene.h"

AwayLogView::AwayLogView(AwayLogFilter *filter, QWidget *parent)
    : ChatMonitorView(filter, parent)
{
    setWindowTitle(tr("Away Log"));
}


void AwayLogView::addActionsToMenu(QMenu *menu, const QPointF &pos)
{
    ChatView::addActionsToMenu(menu, pos);
    if (!menu->isEmpty())
        menu->addSeparator();

    if (scene()->columnByScenePos(pos) == ChatLineModel::SenderColumn) {
        menu->addSeparator();

        QAction *showNetworkAction = menu->addAction(tr("Show Network Name"), this, SLOT(showFieldsChanged(bool)));
        showNetworkAction->setCheckable(true);
        showNetworkAction->setChecked(filter()->showFields() & ChatMonitorFilter::NetworkField);
        showNetworkAction->setData(ChatMonitorFilter::NetworkField);

        QAction *showBufferAction = menu->addAction(tr("Show Buffer Name"), this, SLOT(showFieldsChanged(bool)));
        showBufferAction->setCheckable(true);
        showBufferAction->setChecked(filter()->showFields() & ChatMonitorFilter::BufferField);
        showBufferAction->setData(ChatMonitorFilter::BufferField);
    }
}
