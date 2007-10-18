/***************************************************************************
 *   Copyright (C) 2005-07 by The Quassel IRC Development Team             *
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

#include <QDebug>
#include "chatwidget.h"

ChatWidget::ChatWidget(QWidget *parent) : QTextEdit(parent) {

}

void ChatWidget::setContents(QList<ChatLine *> lines) {
  clear();
  appendChatLines(lines);

}

void ChatWidget::prependMsg(AbstractUiMsg *msg) {
  ChatLine *line = dynamic_cast<ChatLine*>(msg);
  Q_ASSERT(line);
  prependChatLine(line);
}

void ChatWidget::appendMsg(AbstractUiMsg *msg) {
  ChatLine *line = dynamic_cast<ChatLine*>(msg);
  Q_ASSERT(line);
  appendChatLine(line);
}

void ChatWidget::appendChatLine(ChatLine *line) {
  append(line->text()); qDebug() << "appending";
}

void ChatWidget::appendChatLines(QList<ChatLine *> list) {
  foreach(ChatLine *line, list) {
    appendChatLine(line);
  }
}

void ChatWidget::prependChatLine(ChatLine *line) {
  QTextCursor cursor = textCursor();
  moveCursor(QTextCursor::Start);
  insertHtml(line->text());
  setTextCursor(cursor);
}

void ChatWidget::prependChatLines(QList<ChatLine *> list) {


}
