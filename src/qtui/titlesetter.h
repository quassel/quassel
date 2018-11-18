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

#ifndef TITLESETTER_H
#define TITLESETTER_H

#include "abstractitemview.h"

class MainWin;

class TitleSetter : public AbstractItemView
{
    Q_OBJECT

public:
    TitleSetter(MainWin* parent);

protected slots:
    void currentChanged(const QModelIndex& current, const QModelIndex& previous) override;
    void dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight) override;

private:
    MainWin* _mainWin;
    void changeWindowTitle(const QModelIndex& index);
};

#endif
