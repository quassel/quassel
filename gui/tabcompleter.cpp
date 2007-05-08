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

#include "tabcompleter.h"

TabCompleter::TabCompleter(QLineEdit *l, QObject *parent) : QObject(parent) {
  lineEdit = l;
  enabled = false;
}

void TabCompleter::updateNickList(QStringList l) {
  nickList = l;
}

void TabCompleter::updateChannelList(QStringList l) {

}

void TabCompleter::buildCompletionList() {
  // this is the first time tab is pressed -> build up the completion list and it's iterator
  QString tabAbbrev = lineEdit->text().left(lineEdit->cursorPosition()).section(' ',-1,-1);
  completionList.clear();
  foreach(QString nick, nickList) {
    if(nick.toLower().startsWith(tabAbbrev.toLower())) {
      completionList << nick;
    }
  }
  completionList.sort();
  nextCompletion = completionList.begin();
  lastCompletionLength = tabAbbrev.length();
}

void TabCompleter::complete() {
  if (not enabled) {
    buildCompletionList();
    enabled = true;  
  }
  
  if (nextCompletion != completionList.end()) {
    // clear previous completion
    for (int i = 0; i < lastCompletionLength; i++) {
      lineEdit->backspace();
    }
    lineEdit->insert(*nextCompletion + ' ');
    lastCompletionLength = nextCompletion->length() + 1;
    nextCompletion++;
  } else if (completionList.begin() != completionList.end()) {
    nextCompletion = completionList.begin();
  }
  
}

void TabCompleter::disable() {
  enabled = false;
}

