/***************************************************************************
 *   Copyright (C) 2005/06 by the Quassel Project                          *
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

#include "inputline.h"

#include "tabcompleter.h"

InputLine::InputLine(QWidget *parent)
  : QLineEdit(parent),
    idx(0),
    tabCompleter(new TabCompleter(this))
{
  connect(this, SIGNAL(returnPressed()), this, SLOT(on_returnPressed()));
  connect(this, SIGNAL(textChanged(QString)), this, SLOT(on_textChanged(QString)));
}

InputLine::~InputLine() {
}

void InputLine::keyPressEvent(QKeyEvent * event) {
  if(event->key() == Qt::Key_Up) {
    if(idx > 0) { idx--; setText(history[idx]); }
    event->accept();
  } else if(event->key() == Qt::Key_Down) {
    if(idx < history.count()) idx++;
    if(idx < history.count()) setText(history[idx]);
    else if(!text().isEmpty()) {
      history << text();
      idx = history.count();
      setText("");
    }
    event->accept();
  } else if(event->key() == Qt::Key_Select) {  // for Qtopia
    emit returnPressed();
    QLineEdit::keyPressEvent(event);
  } else {
    QLineEdit::keyPressEvent(event);
  }

}

void InputLine::on_returnPressed() {
  history << text();
  idx = history.count();
  emit sendText(text());
  clear();
}

void InputLine::on_textChanged(QString newText) {
  QStringList lineSeperators;
  lineSeperators << QString("\r\n")
		 << QString('\n')
		 << QString('\r');
  
  QString lineSep;
  foreach(QString seperator, lineSeperators) {
    if(newText.contains(seperator)) {
      lineSep = seperator;
      break;
    }
  }

  if(lineSep.isEmpty())
    return;
  
  if(newText.contains(lineSep)) {
    clear();
    QString line = newText.section(lineSep, 0, 0);
    QString remainder = newText.section(lineSep, 1);
    insert(line);
    emit returnPressed();
    insert(remainder);
  }
  
}

