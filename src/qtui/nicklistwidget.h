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

#ifndef _NICKLISTWIDGET_H_
#define _NICKLISTWIDGET_H_

#include "ui_nicklistwidget.h"
#include "types.h"

#include <QHash>

#include <QPointer>
#include <QItemSelectionModel>

#include "buffermodel.h"

class Buffer;
class NickView;

class NickListWidget : public QWidget {
  Q_OBJECT

public:
  NickListWidget(QWidget *parent = 0);

  inline BufferModel *model() { return _bufferModel; }
  void setModel(BufferModel *bufferModel);

  inline QItemSelectionModel *selectionModel() const { return _selectionModel; }
  void setSelectionModel(QItemSelectionModel *selectionModel);

protected:
  virtual QSize sizeHint() const;

protected slots:
//   virtual void closeEditor(QWidget *editor, QAbstractItemDelegate::EndEditHint hint);
//   virtual void commitData(QWidget *editor);
  virtual void currentChanged(const QModelIndex &current, const QModelIndex &previous);
//   virtual void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
//   virtual void editorDestroyed(QObject *editor);
  virtual void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
//   virtual void rowsInserted(const QModelIndex &parent, int start, int end);
//   virtual void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

private slots:
  void removeBuffer(BufferId bufferId);
  
private:
  Ui::NickListWidget ui;
  QHash<BufferId, NickView *> nickViews;

  QPointer<BufferModel> _bufferModel;
  QPointer<QItemSelectionModel> _selectionModel;
};

#endif
