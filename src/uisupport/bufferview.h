/***************************************************************************
 *   Copyright (C) 2005-07 by The Quassel Team                             *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _BUFFERVIEW_H_
#define _BUFFERVIEW_H_

#include <QtGui>
#include <QFlags>

#include "bufferviewfilter.h"

/*****************************************
 * The TreeView showing the Buffers
 *****************************************/
class BufferView : public QTreeView {
  Q_OBJECT
  
public:
  BufferView(QWidget *parent = 0);
  void init();
  void setModel(QAbstractItemModel *model);
  void setFilteredModel(QAbstractItemModel *model, BufferViewFilter::Modes mode, QList<uint> nets);
  
signals:
  void removeBuffer(const QModelIndex &);
  
private slots:
  void joinChannel(const QModelIndex &index);
  void keyPressEvent(QKeyEvent *);
  void rowsInserted (const QModelIndex & parent, int start, int end);
};


#endif

