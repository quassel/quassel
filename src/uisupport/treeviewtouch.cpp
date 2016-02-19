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
#include "treeviewtouch.h"

#include <QtCore>
#include <QTouchEvent>
#include <QScrollBar>

TreeViewTouch::TreeViewTouch(QWidget *parent)
	: QTreeView(parent)
{
	setAttribute(Qt::WA_AcceptTouchEvents);
}


bool TreeViewTouch::event(QEvent *event) {
	if (event->type() == QEvent::TouchBegin) {
		_touchScrollInProgress = true;
		setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
		return true;
	}

	if (event->type() == QEvent::TouchUpdate && _touchScrollInProgress) {
		QTouchEvent::TouchPoint p = ((QTouchEvent*)event)->touchPoints().at(0);
		if (!_firstTouchUpdateHappened) {
			double dx = abs(p.lastPos().x() - p.pos().x());
			double dy = abs(p.lastPos().y() - p.pos().y());
			if (dx > dy) {
				_touchScrollInProgress = false;
			}
			_firstTouchUpdateHappened = true;
		}
		verticalScrollBar()->setValue(verticalScrollBar()->value() - (p.pos().y() - p.lastPos().y()));
		return true;
	}

#if QT_VERSION >= 0x050000
	if (event->type() == QEvent::TouchEnd || event->type() == QEvent::TouchCancel) {
#else
    if (event->type() == QEvent::TouchEnd) {
#endif
		_touchScrollInProgress = false;
		_firstTouchUpdateHappened = false;
		return true;
	}

	return QTreeView::event(event);
}

void TreeViewTouch::mousePressEvent(QMouseEvent * event) {
	if (!_touchScrollInProgress)
		QTreeView::mousePressEvent(event);
}

void TreeViewTouch::mouseMoveEvent(QMouseEvent * event) {
	if (!_touchScrollInProgress)
		QTreeView::mouseMoveEvent(event);
};
