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

#include "debugmessagemodelfilter.h"

#include "messagemodel.h"

DebugMessageModelFilter::DebugMessageModelFilter(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}


QVariant DebugMessageModelFilter::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QVariant();

    switch (section) {
    case 0:
        return "MessageId";
    case 1:
        return "Sender";
    case 2:
        return "Message";
    default:
        return QVariant();
    }
}


QVariant DebugMessageModelFilter::data(const QModelIndex &index, int role) const
{
    if (index.column() != 0 || role != Qt::DisplayRole)
        return QSortFilterProxyModel::data(index, role);

    if (!sourceModel())
        return QVariant();

    QModelIndex source_index = mapToSource(index);
    return sourceModel()->data(source_index, MessageModel::MsgIdRole).value<MsgId>().toInt();
}
