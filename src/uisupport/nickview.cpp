/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
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

#include "nickview.h"

#include <QApplication>
#include <QHeaderView>
#include <QScrollBar>
#include <QDebug>
#include <QMenu>

#include "buffermodel.h"
#include "client.h"
#include "contextmenuactionprovider.h"
#include "graphicalui.h"
#include "nickview.h"
#include "nickviewfilter.h"
#include "networkmodel.h"
#include "types.h"

NickView::NickView(QWidget *parent)
    : TreeViewTouch(parent)
{
    setIndentation(10);
    header()->hide();
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSortingEnabled(true);
    sortByColumn(0, Qt::AscendingOrder);

    setContextMenuPolicy(Qt::CustomContextMenu);
    setSelectionMode(QAbstractItemView::ExtendedSelection);

    setAnimated(true);

    connect(this, &QWidget::customContextMenuRequested, this, &NickView::showContextMenu);

#if defined Q_OS_MACOS || defined Q_OS_WIN
    // afaik this is better on Mac and Windows
    connect(this, SIGNAL(activated(QModelIndex)), SLOT(startQuery(QModelIndex)));
#else
    connect(this, &QAbstractItemView::doubleClicked, this, &NickView::startQuery);
#endif
}


void NickView::init()
{
    if (!model())
        return;

    for (int i = 1; i < model()->columnCount(); i++)
        setColumnHidden(i, true);

    connect(selectionModel(), &QItemSelectionModel::currentChanged, this, &NickView::selectionUpdated);
    connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &NickView::selectionUpdated);
}


void NickView::setModel(QAbstractItemModel *model_)
{
    if (model())
        disconnect(model(), nullptr, this, nullptr);

    TreeViewTouch::setModel(model_);
    init();
}


void NickView::rowsInserted(const QModelIndex &parent, int start, int end)
{
    TreeViewTouch::rowsInserted(parent, start, end);
    if (model()->data(parent, NetworkModel::ItemTypeRole) == NetworkModel::UserCategoryItemType && !isExpanded(parent)) {
        unanimatedExpandAll();
    }
}


void NickView::setRootIndex(const QModelIndex &index)
{
    QAbstractItemView::setRootIndex(index);
    if (index.isValid())
        unanimatedExpandAll();
}


QModelIndexList NickView::selectedIndexes() const
{
    QModelIndexList indexList = TreeViewTouch::selectedIndexes();

    // make sure the item we clicked on is first
    if (indexList.contains(currentIndex())) {
        indexList.removeAll(currentIndex());
        indexList.prepend(currentIndex());
    }

    return indexList;
}


void NickView::unanimatedExpandAll()
{
    // since of Qt Version 4.8.0 the default expandAll will not properly work if
    // animations are enabled. Therefore we perform an unanimated expand when a
    // model is set or a toplevel node is inserted.
    bool wasAnimated = isAnimated();
    setAnimated(false);
    expandAll();
    setAnimated(wasAnimated);
}


void NickView::showContextMenu(const QPoint &pos)
{
    Q_UNUSED(pos);

    QMenu contextMenu(this);
    GraphicalUi::contextMenuActionProvider()->addActions(&contextMenu, selectedIndexes());
    contextMenu.exec(QCursor::pos());
}


void NickView::startQuery(const QModelIndex &index)
{
    if (index.data(NetworkModel::ItemTypeRole) != NetworkModel::IrcUserItemType)
        return;

    auto *ircUser = qobject_cast<IrcUser *>(index.data(NetworkModel::IrcUserRole).value<QObject *>());
    NetworkId networkId = index.data(NetworkModel::NetworkIdRole).value<NetworkId>();
    if (!ircUser || !networkId.isValid())
        return;

    Client::bufferModel()->switchToOrStartQuery(networkId, ircUser->nick());
}
