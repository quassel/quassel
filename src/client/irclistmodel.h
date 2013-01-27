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

#ifndef IRCLISTMODEL_H
#define IRCLISTMODEL_H

#include "irclisthelper.h"

#include <QAbstractItemModel>

class IrcListModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    IrcListModel(QObject *parent = 0);

    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;

    inline QModelIndex parent(const QModelIndex &) const { return QModelIndex(); }

    inline int rowCount(const QModelIndex &parent = QModelIndex()) const { Q_UNUSED(parent) return _channelList.count(); }
    inline int columnCount(const QModelIndex &parent = QModelIndex()) const { Q_UNUSED(parent) return 3; }

public slots:
    void setChannelList(const QList<IrcListHelper::ChannelDescription> &channelList = QList<IrcListHelper::ChannelDescription>());

private:
    QList<IrcListHelper::ChannelDescription> _channelList;
};


#endif //IRCLISTMODEL_H
