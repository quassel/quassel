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

#pragma once

#include "uisupport-export.h"

#include <QTreeView>

/**
 * This class handles Touch Events for TreeViews
 */
class UISUPPORT_EXPORT TreeViewTouch : public QTreeView
{
    Q_OBJECT

public:
    explicit TreeViewTouch(QWidget* parent = nullptr);

protected:
    /**
     * Handles Events
     *
     * @param[in,out] an event
     * @returns true if event got handled, false if event got ignored
     */
    bool event(QEvent* event) override;

    /**
     * Handles Mouse Move Events
     *
     * Suppresses Events during Touch-Scroll
     *
     * @param[in,out] An Event
     */
    void mouseMoveEvent(QMouseEvent* event) override;

    /**
     * Handles Mouse Press Events
     *
     * Suppresses Events during Touch-Scroll
     *
     * @param[in,out] An Event
     */
    void mousePressEvent(QMouseEvent* event) override;

private:
    bool _touchScrollInProgress = false;
    bool _firstTouchUpdateHappened = false;
};
