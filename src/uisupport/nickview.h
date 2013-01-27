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

#ifndef NICKVIEW_H_
#define NICKVIEW_H_

#include <QTreeView>

#include "bufferinfo.h"

class NickView : public QTreeView
{
    Q_OBJECT

public:
    NickView(QWidget *parent = 0);

protected:
    virtual void rowsInserted(const QModelIndex &parent, int start, int end);

    //! This reimplementation ensures that the current index is first in list
    virtual QModelIndexList selectedIndexes() const;

    void unanimatedExpandAll();

public slots:
    virtual void setModel(QAbstractItemModel *model);
    virtual void setRootIndex(const QModelIndex &index);
    void init();
    void showContextMenu(const QPoint &pos);
    void startQuery(const QModelIndex &modelIndex);

signals:
    void selectionUpdated();

private:
    friend class NickListWidget; // needs selectedIndexes()
};


#endif
