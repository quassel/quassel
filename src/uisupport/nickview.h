/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel Project                          *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _NICKVIEW_H_
#define _NICKVIEW_H_


#include <QTreeView>

#include "types.h"
#include "bufferinfo.h"


class NickModel;
class LazySizeHint;
class FilteredNickModel;
class QSortFilterProxyModel;

class NickView : public QTreeView {
  Q_OBJECT

public:
  NickView(QWidget *parent = 0);
  virtual ~NickView();

protected:
  virtual void rowsInserted(const QModelIndex &parent, int start, int end);
  virtual void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
  virtual void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
  virtual QSize sizeHint() const;

public slots:
  void setModel(QAbstractItemModel *model);
  void init();
  void showContextMenu(const QPoint & pos);
  void startQuery(const QModelIndex & modelIndex);
  
private:
  LazySizeHint *_sizeHint;

  BufferInfo bufferInfoFromModelIndex(const QModelIndex & index);
  QString nickFromModelIndex(const QModelIndex & index);
  void executeCommand(const BufferInfo & bufferInfo, const QString & command);
  
};


#endif
