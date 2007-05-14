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

#ifndef _BUFFERWIDGET_H_
#define _BUFFERWIDGET_H_

#include "gui/ui_bufferwidget.h"

#include "global.h"
#include "message.h"
#include "chatwidget.h"

class Buffer;
struct BufferState;
class ChatWidget;
class LayoutThread;

//!\brief Displays the contents of a Buffer.
/** A BufferWidget usually includes a topic line, a nicklist, the chat itself, and an input line.
 * For server buffers or queries, there is of course no nicklist.
 * The contents of the chat is rendered by a ChatWidget.
 */
class BufferWidget : public QWidget {
  Q_OBJECT

  public:
    BufferWidget(QWidget *parent = 0);
    ~BufferWidget();
    void init();

    QSize sizeHint() const;

  signals:
    void userInput(QString msg);
    void aboutToClose();
  
    //void layoutMessages(LayoutTask);
    void nickListUpdated(QStringList l);
      
  protected:

  public slots:
    void setBuffer(Buffer *);
    void saveState();
    //void prependMessages(Buffer *, QList<Message>);  // for backlog processing

  protected:
    void resizeEvent ( QResizeEvent * event );

  private slots:
    void enterPressed();
    void itemExpansionChanged(QTreeWidgetItem *);
    void updateTitle();

    //void displayMsg(Message);
    void updateNickList(BufferState *state, VarMap nicks);
    void updateNickList(VarMap nicks);
    void setOwnNick(QString ownNick);
    void setTopic(QString topic);
    void setActive(bool act = true);

    //void messagesLayouted(LayoutTask);


  private:
    Ui::BufferWidget ui;
    Buffer *curBuf;
    QHash<Buffer *, BufferState *> states;
    bool active;

    ChatWidget *chatWidget;
    QSplitter *splitter;
    QTreeWidget *nickTree;

    QString networkName;
    QString bufferName;

    //LayoutThread *layoutThread;
    //QHash<Buffer *, QList<ChatLine*> > chatLineCache;
    //QHash<Buffer *, QList<Message> > msgCache;
};

struct BufferState {
  ChatWidget *chatWidget;
  QTreeWidget *nickTree;
  QSplitter *splitter;
  QWidget *page;
  Buffer *buffer;
  QByteArray splitterState;
  QString topic, ownNick;
  QString inputLine;
  int currentLine;
  int lineOffset;
  bool opsExpanded, voicedExpanded, usersExpanded;
};

#endif
