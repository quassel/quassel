/***************************************************************************
 *   Copyright (C) 2005-2020 by the Quassel Project                        *
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

#include <QSortFilterProxyModel>

#include "types.h"

class BufferViewOverlay;

class UISUPPORT_EXPORT BufferViewOverlayFilter : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    BufferViewOverlayFilter(QAbstractItemModel* model, BufferViewOverlay* overlay = nullptr);

    void setOverlay(BufferViewOverlay* overlay);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;

private slots:
    void overlayDestroyed();

private:
    BufferViewOverlay* _overlay;
};
