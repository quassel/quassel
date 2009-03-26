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

#include "bufferview.h"
#include "graphicalui.h"
#include "inputline.h"
#include "tabcompleter.h"

InputLine::InputLine(QWidget *parent)
  :
#ifdef HAVE_KDE
    KTextEdit(parent),
#else
    QLineEdit(parent),
#endif
    idx(0),
    tabCompleter(new TabCompleter(this))
{
#ifdef HAVE_KDE
//This is done to make the KTextEdit look like a lineedit
  setMaximumHeight(document()->size().toSize().height());
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setAcceptRichText(false);
  setLineWrapMode(NoWrap);
  enableFindReplace(false);
  connect(this, SIGNAL(textChanged()), this, SLOT(on_textChanged()));
#endif

  connect(this, SIGNAL(returnPressed()), this, SLOT(on_returnPressed()));
  connect(this, SIGNAL(textChanged(QString)), this, SLOT(on_textChanged(QString)));
}

InputLine::~InputLine() {
}

void InputLine::setCustomFont(const QFont &font) {
  setFont(font);
#ifdef HAVE_KDE
  setMaximumHeight(document()->size().toSize().height());
#endif
}

bool InputLine::eventFilter(QObject *watched, QEvent *event) {
  if(event->type() != QEvent::KeyPress)
    return false;

  // keys from BufferView should be sent to (and focus) the input line
  BufferView *view = qobject_cast<BufferView *>(watched);
  if(view) {
    QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
    if(keyEvent->text().length() == 1 && !(keyEvent->modifiers() & (Qt::ControlModifier ^ Qt::AltModifier)) ) { // normal key press
      QChar c = keyEvent->text().at(0);
      if(c.isLetterOrNumber() || c.isSpace() || c.isPunct() || c.isSymbol()) {
        setFocus();
        keyPressEvent(keyEvent);
        return true;
      } else
        return false;
    }
  }
  return false;
}

void InputLine::keyPressEvent(QKeyEvent * event) {

#ifdef HAVE_KDE
  if(event->matches(QKeySequence::Find)) {
    QAction *act = GraphicalUi::actionCollection()->action("ToggleSearchBar");
    if(act) {
      act->toggle();
      event->accept();
      return;
    }
  }
#endif

  switch(event->key()) {
  case Qt::Key_Up:
    event->accept();

    addToHistory(text(), true);

    if(idx > 0) {
      idx--;
      showHistoryEntry();
    }

    break;

  case Qt::Key_Down:
    event->accept();

    addToHistory(text(), true);

    if(idx < history.count()) {
      idx++;
      if(idx < history.count() || tempHistory.contains(idx)) // tempHistory might have an entry for idx == history.count() + 1
        showHistoryEntry();
      else
        resetLine();              // equals clear() in this case
    } else {
      addToHistory(text());
      resetLine();
    }

    break;

  case Qt::Key_Select:		// for Qtopia
    emit returnPressed();
    break;

#ifdef HAVE_KDE
//Since this is a ktextedit, we don't have this signal "natively"
  case Qt::Key_Return:
  case Qt::Key_Enter:
    event->accept();
    emit returnPressed();
    break;

#endif

  default:
#ifdef HAVE_KDE
    KTextEdit::keyPressEvent(event);
#else
    QLineEdit::keyPressEvent(event);
#endif
  }
}

bool InputLine::addToHistory(const QString &text, bool temporary) {
  if(text.isEmpty())
    return false;

  Q_ASSERT(0 <= idx && idx <= history.count());

  if(history.isEmpty() || text != history[idx - (int)(idx == history.count())]) {
    // if an entry of the history is changed, we remember it and show it again at this
    // position until a line was actually sent
    // sent lines get appended to the history
    if(temporary) {
      tempHistory[idx] = text;
    } else {
      history << text;
      tempHistory.clear();
    }
    return true;
  } else {
    return false;
  }
}

void InputLine::on_returnPressed() {
  if(!text().isEmpty()) {
    addToHistory(text());
    emit sendText(text());
    resetLine();
  }
}

void InputLine::on_textChanged(QString newText) {
  QStringList lineSeparators;
  lineSeparators << QString("\r\n")
		 << QString('\n')
		 << QString('\r');

  QString lineSep;
  foreach(QString separator, lineSeparators) {
    if(newText.contains(separator)) {
      lineSep = separator;
      break;
    }
  }

  if(lineSep.isEmpty())
    return;

  QStringList lines = newText.split(lineSep);
  clear();

  if(lines.count() >= 4) {
    QString msg = tr("Do you really want to paste %n lines?", "", lines.count());
    msg += "<p>";
    for(int i = 0; i < 3; i++) {
      msg += lines[i].left(40);
      if(lines[i].count() > 40)
	msg += "...";
      msg += "<br />";
    }
    msg += "...</p>";
    QMessageBox question(QMessageBox::NoIcon, tr("Paste Protection"), msg, QMessageBox::Yes|QMessageBox::No);
    question.setDefaultButton(QMessageBox::No);
#ifdef Q_WS_MAC
    question.setWindowFlags(question.windowFlags() | Qt::Sheet);
#endif
    if(question.exec() == QMessageBox::No)
      return;
  }

  foreach(QString line, lines) {
    if(!line.isEmpty()) {
      clear();
      insert(line);
      emit returnPressed();
    }
  }
//   if(newText.contains(lineSep)) {
//     clear();
//     QString line = newText.section(lineSep, 0, 0);
//     QString remainder = newText.section(lineSep, 1);
//     insert(line);
//     emit returnPressed();
//     insert(remainder);
//   }
}

void InputLine::resetLine() {
  // every time the InputLine is cleared we also reset history index
  idx = history.count();
  clear();
}

void InputLine::showHistoryEntry() {
  // if the user changed the history, display the changed line
  tempHistory.contains(idx) ? setText(tempHistory[idx]) : setText(history[idx]);
}
