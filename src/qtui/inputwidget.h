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

#ifndef INPUTWIDGET_H
#define INPUTWIDGET_H

#include "ui_inputwidget.h"
#include <QPointer>
#include <QItemSelectionModel>

#include "buffermodel.h"
#include "bufferinfo.h"
#include "identity.h"
#include "network.h";

class InputWidget : public QWidget {
  Q_OBJECT

public:
  InputWidget(QWidget *parent = 0);
  virtual ~InputWidget();

  inline BufferModel *model() { return _bufferModel; }
  void setModel(BufferModel *bufferModel);

  inline QItemSelectionModel *selectionModel() const { return _selectionModel; }
  void setSelectionModel(QItemSelectionModel *selectionModel);

  const Network *currentNetwork() const;

protected slots:
//   virtual void closeEditor(QWidget *editor, QAbstractItemDelegate::EndEditHint hint);
//   virtual void commitData(QWidget *editor);
  virtual void currentChanged(const QModelIndex &current, const QModelIndex &previous);
//   virtual void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
//   virtual void editorDestroyed(QObject *editor);
//   virtual void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
//   virtual void rowsInserted(const QModelIndex &parent, int start, int end);
//   virtual void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

private slots:
  void sendText(QString text);
  void changeNick(const QString &newNick) const;

  void setNetwork(const Network *network);
  void setIdentity(const IdentityId &identityId);
  void updateNickSelector() const;

signals:
  void userInput(BufferInfo, QString msg) const;

private:
  Ui::InputWidget ui;

  bool validBuffer;
  BufferInfo currentBufferInfo;
  
  QPointer<BufferModel> _bufferModel;
  QPointer<QItemSelectionModel> _selectionModel;
  NetworkId _networkId;
  IdentityId _identityId;

};

#endif // INPUTWIDGET_H
