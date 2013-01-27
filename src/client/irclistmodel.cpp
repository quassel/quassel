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

#include "irclistmodel.h"

#include <QStringList>

IrcListModel::IrcListModel(QObject *parent)
    : QAbstractItemModel(parent)
{
}


QVariant IrcListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount() || index.column() >= columnCount() || role != Qt::DisplayRole)
        return QVariant();

    IrcListHelper::ChannelDescription channel = _channelList[index.row()];

    switch (index.column()) {
    case 0:
        return channel.channelName;
    case 1:
        return channel.userCount;
    case 2:
        return channel.topic;
    default:
        return QVariant();
    }
}


Qt::ItemFlags IrcListModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return Qt::ItemIsDropEnabled;
    }
    else {
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    }
}


QVariant IrcListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    QStringList header;
    header << tr("Channel")
           << tr("Users")
           << tr("Topic");

    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return header[section];

    return QVariant();
}


QModelIndex IrcListModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    if (row >= rowCount() || column >= columnCount())
        return QModelIndex();

    return createIndex(row, column);
}


void IrcListModel::setChannelList(const QList<IrcListHelper::ChannelDescription> &channelList)
{
    if (rowCount() > 0) {
        beginRemoveRows(QModelIndex(), 0, _channelList.count() - 1);
        _channelList.clear();
        endRemoveRows();
    }

    if (channelList.count() > 0) {
        beginInsertRows(QModelIndex(), 0, channelList.count() - 1);
        _channelList = channelList;
        endInsertRows();
    }
}
