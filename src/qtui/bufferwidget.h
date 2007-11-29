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

#ifndef _BUFFERWIDGET_H_
#define _BUFFERWIDGET_H_

#include "ui_bufferwidget.h"

#include "chatview.h"
#include "types.h"

class Buffer;
class ChatView;
class ChatWidget;
class LayoutThread;

//! Displays the contents of a Buffer.
/**
*/
class BufferWidget : public QWidget {
  Q_OBJECT

public:
  BufferWidget(QWidget *parent = 0);
  virtual ~BufferWidget();
  void init();

  QSize sizeHint() const;

signals:
  void userInput(QString msg);
  void aboutToClose();

public slots:
  void setBuffer(Buffer *);
  void saveState();

private slots:
  void enterPressed();
  void setActive(bool act = true);


private:
  Ui::BufferWidget ui;
  //QHash<BufferId, ChatView*> _chatViews;
  QHash<BufferId, ChatWidget *> _chatWidgets;
  bool active;

  QString _networkName;
  QString _bufferName;
};


#endif
