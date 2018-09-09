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

#include "client-export.h"

#include "irclisthelper.h"

#include <QAbstractItemModel>

class CLIENT_EXPORT IrcListModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    IrcListModel(QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;

    inline QModelIndex parent(const QModelIndex &) const override { return {}; }

    inline int rowCount(const QModelIndex &parent = QModelIndex()) const override { Q_UNUSED(parent) return _channelList.count(); }
    inline int columnCount(const QModelIndex &parent = QModelIndex()) const override { Q_UNUSED(parent) return 3; }

public slots:
    void setChannelList(const QList<IrcListHelper::ChannelDescription> &channelList = QList<IrcListHelper::ChannelDescription>());

private:
    QList<IrcListHelper::ChannelDescription> _channelList;
};
