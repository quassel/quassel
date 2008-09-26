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

#ifndef _MAINWIDGET_H_
#define _MAINWIDGET_H_

#include "ui_mainwidget.h"

#include "abstractbuffercontainer.h"
#include "bufferinfo.h"

class Buffer;
class ChatWidget;

class MainWidget : public AbstractBufferContainer {
  Q_OBJECT

  public:
    MainWidget(QWidget *parent);
    ~MainWidget();

  signals:
    void userInput(const BufferInfo &, const QString &);

  protected:
    virtual AbstractChatView *createChatView(BufferId);
    virtual void removeChatView(BufferId);

  protected slots:
    virtual void showChatView(BufferId);

  private slots:
    void userInput(const QString &);

  private:
    Ui::MainWidget ui;
    QHash<BufferId, ChatWidget *> _chatViews;
    BufferInfo currentBufferInfo;

};


#endif
