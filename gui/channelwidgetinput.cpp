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

#include "channelwidgetinput.h"
#include <QCompleter>

ChannelWidgetInput::ChannelWidgetInput(QWidget *parent) : QLineEdit(parent) {
  idx = 0;
  tabMode = false;
  connect(this, SIGNAL(returnPressed()), this, SLOT(enter()));
}

void ChannelWidgetInput::keyPressEvent(QKeyEvent * event) {
  if(event->key() == Qt::Key_Tab) {
    // Tabcomplete
    if(text().length() > 0) {
      if (not tabMode) {
        QString tabAbbrev = text().left(cursorPosition()).section(' ',-1,-1);
        tabCompleteList.clear();
        foreach(QString nick, nickList) {
          if(nick.toLower().startsWith(tabAbbrev.toLower())) {
            tabCompleteList << nick;
          }
        }
        
        tabCompleteList.sort();
        lastCompletionLength = tabAbbrev.length();
        tabMode = true;
        nextCompletion = tabCompleteList.begin();
      }
      if (nextCompletion != tabCompleteList.end()) {
        for (int i = 0; i < lastCompletionLength; i++) {
          backspace();
        }
        insert(*nextCompletion);
        lastCompletionLength = nextCompletion->length();
        nextCompletion++;
      } else if (tabCompleteList.end() != tabCompleteList.begin()) {
        nextCompletion = tabCompleteList.begin();
      }
    }
    event->accept();
    
  } else {
    tabMode = false;
    if(event->key() == Qt::Key_Up) {
      if(idx > 0) { idx--; setText(history[idx]); }
      event->accept();
    } else if(event->key() == Qt::Key_Down) {
      if(idx < history.count()) idx++;
      if(idx < history.count()) setText(history[idx]);
      else setText("");
      event->accept();
    } else {
      QLineEdit::keyPressEvent(event);
    }
  }
}

bool ChannelWidgetInput::event(QEvent *e) {
  if(e->type() == QEvent::KeyPress) {
    keyPressEvent(dynamic_cast<QKeyEvent*>(e));
    return true;
  }
  return QLineEdit::event(e);
}

void ChannelWidgetInput::enter() {
  history << text();
  idx = history.count();
}

void ChannelWidgetInput::updateNickList(QStringList l) {
  nickList = l;
}
