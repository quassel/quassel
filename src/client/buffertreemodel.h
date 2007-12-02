/***************************************************************************
 *   Copyright (C) 2005-07 by the Quassel IRC Team                         *
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

#ifndef _BUFFERTREEMODEL_H_
#define _BUFFERTREEMODEL_H_

#include <QtCore>

#include "treemodel.h"
#include "buffer.h"

#include <QPointer>

#include <QItemSelectionModel>

class BufferInfo;

#include "selectionmodelsynchronizer.h"
#include "modelpropertymapper.h"
class MappedSelectionModel;
class QAbstractItemView;

/*****************************************
 *  Fancy Buffer Items
 *****************************************/
class BufferTreeItem : public TreeItem {
  Q_OBJECT

public:
  BufferTreeItem(Buffer *, TreeItem *parent = 0);

  virtual quint64 id() const;
  QVariant data(int column, int role) const;

  Buffer *buffer() const { return buf; }
  void setActivity(const Buffer::ActivityLevel &);

private:
  QString text(int column) const;
  QColor foreground(int column) const;

  Buffer *buf;
  Buffer::ActivityLevel activity;
};

/*****************************************
 *  Network Items
 *****************************************/
class NetworkTreeItem : public TreeItem {
  Q_OBJECT

public:
  NetworkTreeItem(const uint &netid, const QString &, TreeItem *parent = 0);

  virtual QVariant data(int column, int row) const;
  virtual quint64 id() const;

private:
  uint _networkId;
  QString net;
};

/*****************************************
 * BufferTreeModel
 *****************************************/
class BufferTreeModel : public TreeModel {
  Q_OBJECT

public:
  enum myRoles {
    BufferTypeRole = Qt::UserRole,
    BufferActiveRole,
    BufferUidRole,
    NetworkIdRole
  };

  BufferTreeModel(QObject *parent = 0);
  static QList<QVariant> defaultHeader();

  inline SelectionModelSynchronizer *selectionModelSynchronizer() { return _selectionModelSynchronizer; }
  inline ModelPropertyMapper *propertyMapper() { return _propertyMapper; }

  void synchronizeSelectionModel(MappedSelectionModel *selectionModel);
  void synchronizeView(QAbstractItemView *view);
  void mapProperty(int column, int role, QObject *target, const QByteArray &property);

  static bool mimeContainsBufferList(const QMimeData *mimeData);
  static QList< QPair<uint, uint> > mimeDataToBufferList(const QMimeData *mimeData);

  virtual QStringList mimeTypes() const;
  virtual QMimeData *mimeData(const QModelIndexList &) const;
  virtual bool dropMimeData(const QMimeData *, Qt::DropAction, int, int, const QModelIndex &);


public slots:
  void bufferUpdated(Buffer *);
  void setCurrentIndex(const QModelIndex &index, QItemSelectionModel::SelectionFlags command);
  void selectBuffer(Buffer *buffer);
  void bufferActivity(Buffer::ActivityLevel, Buffer *buffer);

signals:
  void bufferSelected(Buffer *);
  void invalidateFilter();
  void selectionChanged(const QModelIndex &);

private:
  bool isBufferIndex(const QModelIndex &) const;
  Buffer *getBufferByIndex(const QModelIndex &) const;
  QModelIndex getOrCreateNetworkItemIndex(Buffer *buffer);
  QModelIndex getOrCreateBufferItemIndex(Buffer *buffer);

  QPointer<SelectionModelSynchronizer> _selectionModelSynchronizer;
  QPointer<ModelPropertyMapper> _propertyMapper;
  Buffer *currentBuffer;
};

#endif
