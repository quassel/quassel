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

#include <QSortFilterProxyModel>

#include "types.h"

class NetworkModel;

// This is proxymodel is purely for the sorting right now
class UISUPPORT_EXPORT NickViewFilter : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    NickViewFilter(const BufferId &bufferId, NetworkModel *parent = nullptr);

    virtual QVariant data(const QModelIndex &index, int role) const;
    QVariant icon(const QModelIndex &index) const;

protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
    QVariant styleData(const QModelIndex &index, int role) const;

private:
    BufferId _bufferId;
};
