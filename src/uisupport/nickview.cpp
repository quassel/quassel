/***************************************************************************
 *   Copyright (C) 2005-2015 by the Quassel Project                        *
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
#include <QTouchEvent>
#include <QScrollBar>

#include "buffermodel.h"
#include "client.h"
#include "contextmenuactionprovider.h"
#include "graphicalui.h"
#include "nickview.h"
#include "nickviewfilter.h"
#include "networkmodel.h"
#include "types.h"

NickView::NickView(QWidget *parent)
    : QTreeView(parent)
{
    setIndentation(10);
    header()->hide();
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSortingEnabled(true);
    sortByColumn(0, Qt::AscendingOrder);

    setContextMenuPolicy(Qt::CustomContextMenu);
    setSelectionMode(QAbstractItemView::ExtendedSelection);

//   // breaks with Qt 4.8
//   if(QString("4.8.0") > qVersion()) // FIXME breaks with Qt versions >= 4.10!
    setAnimated(true);

    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), SLOT(showContextMenu(const QPoint &)));

#if defined Q_WS_QWS || defined Q_WS_X11
    connect(this, SIGNAL(doubleClicked(QModelIndex)), SLOT(startQuery(QModelIndex)));
#else
    // afaik this is better on Mac and Windows
    connect(this, SIGNAL(activated(QModelIndex)), SLOT(startQuery(QModelIndex)));
#endif
}


void NickView::init()
{
    if (!model())
        return;

    for (int i = 1; i < model()->columnCount(); i++)
        setColumnHidden(i, true);

    connect(selectionModel(), SIGNAL(currentChanged(QModelIndex, QModelIndex)), SIGNAL(selectionUpdated()));
    connect(selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), SIGNAL(selectionUpdated()));
	setAttribute(Qt::WA_AcceptTouchEvents);
}


void NickView::setModel(QAbstractItemModel *model_)
{
    if (model())
        disconnect(model(), 0, this, 0);

    QTreeView::setModel(model_);
    init();
}


void NickView::rowsInserted(const QModelIndex &parent, int start, int end)
{
    QTreeView::rowsInserted(parent, start, end);
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
    QModelIndexList indexList = QTreeView::selectedIndexes();

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

    IrcUser *ircUser = qobject_cast<IrcUser *>(index.data(NetworkModel::IrcUserRole).value<QObject *>());
    NetworkId networkId = index.data(NetworkModel::NetworkIdRole).value<NetworkId>();
    if (!ircUser || !networkId.isValid())
        return;

    Client::bufferModel()->switchToOrStartQuery(networkId, ircUser->nick());
}

bool NickView::event(QEvent *event) {
	if (event->type() == QEvent::TouchBegin && _lastTouchStart < QDateTime::currentMSecsSinceEpoch() - 1000) { //(slow) double tab = normal behaviour = select multiple. 1000 ok?
		_touchScrollInProgress = true;
		_lastTouchStart = QDateTime::currentMSecsSinceEpoch();
		setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
		return true;
	}

	if (event->type() == QEvent::TouchUpdate && _touchScrollInProgress) {
		QTouchEvent::TouchPoint p = ((QTouchEvent*)event)->touchPoints().at(0);
		verticalScrollBar()->setValue(verticalScrollBar()->value() - (p.pos().y() - p.lastPos().y()));
		return true;
	}

#if QT_VERSION >= 0x050000
	if (event->type() == QEvent::TouchEnd || event->type() == QEvent::TouchCancel) {
#else
    if (event->type() == QEvent::TouchEnd) {
#endif
		_touchScrollInProgress = false;
		return true;
	}

	return QTreeView::event(event);
}

void NickView::mousePressEvent(QMouseEvent * event) {
	if (!_touchScrollInProgress)
		QTreeView::mousePressEvent(event);
}

void NickView::mouseMoveEvent(QMouseEvent * event) {
	if (!_touchScrollInProgress)
		QTreeView::mouseMoveEvent(event);
}
