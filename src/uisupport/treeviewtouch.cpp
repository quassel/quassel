/***************************************************************************
 *   Copyright (C) 2005-2022 by the Quassel Project                        *
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

#include <QEvent>
#include <QScrollBar>
#include <QTouchEvent>

TreeViewTouch::TreeViewTouch(QWidget* parent)
    : QTreeView(parent)
{
    setAttribute(Qt::WA_AcceptTouchEvents);
}

bool TreeViewTouch::event(QEvent* event)
{
    if (event->type() == QEvent::TouchBegin && ((QTouchEvent*)event)->device()->type() == QTouchDevice::TouchScreen) {
        // Register that we may be scrolling, set the scroll mode to scroll-per-pixel
        // and accept the event (return true) so that we will receive TouchUpdate and TouchEnd/TouchCancel
        _touchScrollInProgress = true;
        setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        return true;
    }

    if (event->type() == QEvent::TouchUpdate && _touchScrollInProgress) {
        QTouchEvent::TouchPoint p = ((QTouchEvent*)event)->touchPoints().at(0);
        if (!_firstTouchUpdateHappened) {
            // After the first movement of a Touch-Point, calculate the distance in both axis
            // and if the point moved more horizontally abort scroll.
            double dx = qAbs(p.lastPos().x() - p.pos().x());
            double dy = qAbs(p.lastPos().y() - p.pos().y());
            if (dx > dy) {
                _touchScrollInProgress = false;
            }
            _firstTouchUpdateHappened = true;
        }
        // Apply touch movement to scrollbar
        verticalScrollBar()->setValue(verticalScrollBar()->value() - (p.pos().y() - p.lastPos().y()));
        return true;
    }

    if (event->type() == QEvent::TouchEnd || event->type() == QEvent::TouchCancel) {
        // End scroll and reset variables
        _touchScrollInProgress = false;
        _firstTouchUpdateHappened = false;
        return true;
    }

    return QTreeView::event(event);
}

void TreeViewTouch::mousePressEvent(QMouseEvent* event)
{
    if (!_touchScrollInProgress)
        QTreeView::mousePressEvent(event);
}

void TreeViewTouch::mouseMoveEvent(QMouseEvent* event)
{
    if (!_touchScrollInProgress)
        QTreeView::mouseMoveEvent(event);
}
