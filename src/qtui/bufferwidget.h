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

#ifndef _BUFFERWIDGET_H_
#define _BUFFERWIDGET_H_

#include "ui_bufferwidget.h"

#include "chatview.h"
#include "types.h"

class Network;
class ChatView;
class ChatWidget;

#include "buffermodel.h"
#include <QItemSelectionModel>

//! Displays the contents of a Buffer.
/**
*/
class BufferWidget : public QWidget {
  Q_OBJECT

public:
  BufferWidget(QWidget *parent = 0);
  virtual ~BufferWidget();
  void init();

  inline BufferModel *model() { return _bufferModel; }
  void setModel(BufferModel *bufferModel);

  inline QItemSelectionModel *selectionModel() const { return _selectionModel; }
  void setSelectionModel(QItemSelectionModel *selectionModel);

  inline BufferId currentBuffer() const { return _currentBuffer; }
  
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
  void setCurrentBuffer(BufferId bufferId);

private:
  Ui::BufferWidget ui;
  QHash<BufferId, ChatWidget *> _chatWidgets;

  QPointer<BufferModel> _bufferModel;
  QPointer<QItemSelectionModel> _selectionModel;

  BufferId _currentBuffer;
};

#endif
