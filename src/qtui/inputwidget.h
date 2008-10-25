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

#include "abstractitemview.h"
#include "buffermodel.h"
#include "bufferinfo.h"
#include "identity.h"
#include "network.h"

class InputLine;

class InputWidget : public AbstractItemView {
  Q_OBJECT

public:
  InputWidget(QWidget *parent = 0);
  virtual ~InputWidget();

  const Network *currentNetwork() const;

  inline  InputLine* inputLine() const { return ui.inputEdit; }

protected slots:
  virtual void currentChanged(const QModelIndex &current, const QModelIndex &previous);
  virtual void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);

private slots:
  void sendText(QString text);
  void changeNick(const QString &newNick) const;

  void setNetwork(const Network *network);
  void setIdentity(const IdentityId &identityId);
  void updateNickSelector() const;
  void updateEnabledState();

  BufferInfo currentBufferInfo() const;

signals:
  void userInput(BufferInfo, QString msg) const;

private:
  Ui::InputWidget ui;
  
  NetworkId _networkId;
  IdentityId _identityId;
};


class MouseWheelFilter : public QObject {
  Q_OBJECT

public:
  MouseWheelFilter(QObject *parent = 0);
  virtual bool eventFilter(QObject *obj, QEvent *event);
};

#endif // INPUTWIDGET_H
