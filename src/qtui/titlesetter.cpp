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

#include "titlesetter.h"

#include "abstractitemview.h"
#include "client.h"
#include "mainwin.h"

TitleSetter::TitleSetter(MainWin *parent)
    : AbstractItemView(parent),
    _mainWin(parent)
{
}


void TitleSetter::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous);
    changeWindowTitle(current.sibling(current.row(), 0));
}


void TitleSetter::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    QItemSelectionRange changedArea(topLeft, bottomRight);
    QModelIndex currentTopicIndex = selectionModel()->currentIndex().sibling(selectionModel()->currentIndex().row(), 0);
    if (changedArea.contains(currentTopicIndex))
        changeWindowTitle(currentTopicIndex);
};

void TitleSetter::changeWindowTitle(const QModelIndex &index)
{
    BufferId id = index.data(NetworkModel::BufferIdRole).value<BufferId>();
    if (!id.isValid())
        return;

    QString title;
    if (Client::networkModel()->bufferType(id) == BufferInfo::StatusBuffer)
        title = index.data().toString();
    else
        title = QString("%1 (%2)").arg(index.data().toString(), Client::networkModel()->networkName(id));
    QString newTitle = QString("%1 - %2").arg("Quassel IRC").arg(title);

    _mainWin->setWindowTitle(newTitle);
    _mainWin->setWindowIconText(newTitle);
}
