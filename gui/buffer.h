/***************************************************************************
 *   Copyright (C) 2005/06 by The Quassel Team                             *
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

#ifndef _BUFFER_H_
#define _BUFFER_H_

#include <QtCore>
#include <QtGui>

#include "ui_bufferwidget.h"

#include "global.h"
#include "message.h"

class ChatWidget;
class ChatWidgetContents;
class BufferWidget;

//!\brief Encapsulates the contents of a single channel, query or server status context.
/** A Buffer maintains a list of existing nicks and their status. New messages can be appended using
 * displayMsg(). A buffer displays its contents by way of a BufferWidget, which can be shown
 * (and created on demand) by calling showWidget().
 */
class Buffer : public QObject {
  Q_OBJECT

  public:
    Buffer(QString network, QString buffer);
    ~Buffer();

    bool isActive() { return active; }
  public:
    QWidget *getWidget();

  signals:
    void userInput(QString, QString, QString);
    void nickListChanged(QStringList);

  public slots:
    void setActive(bool active = true);
    void displayMsg(Message);
    //void recvStatusMsg(QString msg);
    void setTopic(QString);
    //void setNicks(QStringList);
    void addNick(QString nick, VarMap props);
    void renameNick(QString oldnick, QString newnick);
    void removeNick(QString nick);
    void updateNick(QString nick, VarMap props);
    void setOwnNick(QString nick);

    QWidget * showWidget(QWidget *parent = 0);
    void hideWidget();
    void deleteWidget();

    void scrollToEnd();

  private slots:
    void userInput(QString);

  private:
    bool active;
    BufferWidget *widget;
    ChatWidget *chatWidget;
    ChatWidgetContents *contentsWidget;
    VarMap nicks;
    QString topic;
    QString ownNick;
    QString networkName, bufferName;

    QList<Message> contents;
};

//!\brief Displays the contents of a Buffer.
/** A BufferWidget usually includes a topic line, a nicklist, the chat itself, and an input line.
 * For server buffers or queries, there is of course no nicklist.
 * The contents of the chat is rendered by a ChatWidget.
 */
class BufferWidget : public QWidget {
  Q_OBJECT

  public:
    BufferWidget(QString netname, QString bufname, bool active, QString ownNick, ChatWidgetContents *contents, Buffer *parentBuffer, QWidget *parent = 0);
    ~BufferWidget();

    void setActive(bool act = true);

  signals:
    void userInput(QString);

  protected:

  public slots:
    void displayMsg(Message);
    void updateNickList(VarMap nicks);
    void setOwnNick(QString ownNick);
    void setTopic(QString topic);

  private slots:
    void enterPressed();
    void itemExpansionChanged(QTreeWidgetItem *);
    void updateTitle();

  private:
    Ui::BufferWidget ui;
    Buffer *parentBuffer;
    bool active;

    QString networkName;
    QString bufferName;

    bool opsExpanded, voicedExpanded, usersExpanded;

};

#endif
