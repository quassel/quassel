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

#ifndef TREEVIEWTOUCH_H_
#define TREEVIEWTOUCH_H_

#include <QTreeView>
/**
* This class handles Touch Events for TreeViews
*/
class TreeViewTouch :
    public QTreeView
{
    Q_OBJECT

public:
    explicit TreeViewTouch(QWidget *parent = 0);

protected:

    /**
    * Handles Events
    *
    * @param[in,out] an event
    * @returns true if event got handled, false if event got ignored
    */
    virtual bool event(QEvent *event);

    /**
    * Handles Mouse Move Events
    *
    * Suppresses Events during Touch-Scroll
    *
    * @param[in,out] An Event
    */
    virtual void mouseMoveEvent(QMouseEvent *event);

    /**
    * Handles Mouse Press Events
    *
    * Suppresses Events during Touch-Scroll
    *
    * @param[in,out] An Event
    */
    virtual void mousePressEvent(QMouseEvent *event);

private:
    bool _touchScrollInProgress = false;
    bool _firstTouchUpdateHappened = false;
};

#endif