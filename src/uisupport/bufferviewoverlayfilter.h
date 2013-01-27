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

#ifndef BUFFERVIEWOVERLAYFILTER_H_
#define BUFFERVIEWOVERLAYFILTER_H_

#include <QSortFilterProxyModel>

#include "types.h"

class BufferViewOverlay;

class BufferViewOverlayFilter : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    BufferViewOverlayFilter(QAbstractItemModel *model, BufferViewOverlay *overlay = 0);

    void setOverlay(BufferViewOverlay *overlay);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;

private slots:
    void overlayDestroyed();

private:
    BufferViewOverlay *_overlay;
};


#endif // BUFFERVIEWOVERLAYFILTER_H_
