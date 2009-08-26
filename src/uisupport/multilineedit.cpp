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

#include <QApplication>
#include <QMenu>
#include <QMessageBox>
#include <QScrollBar>

#include "bufferview.h"
#include "graphicalui.h"
#include "multilineedit.h"
#include "tabcompleter.h"

const int leftMargin = 3;

MultiLineEdit::MultiLineEdit(QWidget *parent)
  :
#ifdef HAVE_KDE
    KTextEdit(parent),
#else
    QTextEdit(parent),
#endif
    idx(0),
    _mode(SingleLine),
    _singleLine(true),
    _minHeight(1),
    _maxHeight(5),
    _scrollBarsEnabled(true),
    _lastDocumentHeight(-1)
{
#if QT_VERSION >= 0x040500
  document()->setDocumentMargin(0); // new in Qt 4.5 and we really don't want it here
#endif

  setAcceptRichText(false);
#ifdef HAVE_KDE
  enableFindReplace(false);
#endif

  setMode(SingleLine);
  setWordWrapEnabled(false);
  reset();

  connect(this, SIGNAL(textChanged()), this, SLOT(on_textChanged()));
}

MultiLineEdit::~MultiLineEdit() {
}

void MultiLineEdit::setCustomFont(const QFont &font) {
  setFont(font);
  updateSizeHint();
}

void MultiLineEdit::setMode(Mode mode) {
  if(mode == _mode)
    return;

  _mode = mode;
}

void MultiLineEdit::setMinHeight(int lines) {
  if(lines == _minHeight)
    return;

  _minHeight = lines;
  updateSizeHint();
}

void MultiLineEdit::setMaxHeight(int lines) {
  if(lines == _maxHeight)
    return;

  _maxHeight = lines;
  updateSizeHint();
}

void MultiLineEdit::setScrollBarsEnabled(bool enable) {
  if(_scrollBarsEnabled == enable)
    return;

  _scrollBarsEnabled = enable;
  updateScrollBars();
}

void MultiLineEdit::updateScrollBars() {
  QFontMetrics fm(font());
  int _maxPixelHeight = fm.lineSpacing() * _maxHeight;
  if(_scrollBarsEnabled && document()->size().height() > _maxPixelHeight)
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  else
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  if(!_scrollBarsEnabled || isSingleLine())
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  else
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

void MultiLineEdit::resizeEvent(QResizeEvent *event) {
  QTextEdit::resizeEvent(event);
  updateSizeHint();
  updateScrollBars();
}

void MultiLineEdit::updateSizeHint() {
  QFontMetrics fm(font());
  int minPixelHeight = fm.lineSpacing() * _minHeight;
  int maxPixelHeight = fm.lineSpacing() * _maxHeight;
  int scrollBarHeight = horizontalScrollBar()->isVisible() ? horizontalScrollBar()->height() : 0;

  // use the style to determine a decent size
  int h = qMin(qMax((int)document()->size().height() + scrollBarHeight, minPixelHeight), maxPixelHeight) + 2 * frameWidth();
  QStyleOptionFrameV2 opt;
  opt.initFrom(this);
  opt.rect = QRect(0, 0, 100, h);
  opt.lineWidth = lineWidth();
  opt.midLineWidth = midLineWidth();
  opt.state |= QStyle::State_Sunken;
  QSize s = style()->sizeFromContents(QStyle::CT_LineEdit, &opt, QSize(100, h).expandedTo(QApplication::globalStrut()), this);
  if(s != _sizeHint) {
    _sizeHint = s;
    updateGeometry();
  }
}

QSize MultiLineEdit::sizeHint() const {
  if(!_sizeHint.isValid()) {
    MultiLineEdit *that = const_cast<MultiLineEdit *>(this);
    that->updateSizeHint();
  }
  return _sizeHint;
}

QSize MultiLineEdit::minimumSizeHint() const {
  return sizeHint();
}

void MultiLineEdit::setSpellCheckEnabled(bool enable) {
#ifdef HAVE_KDE
  setCheckSpellingEnabled(enable);
#else
  Q_UNUSED(enable)
#endif
}

void MultiLineEdit::setWordWrapEnabled(bool enable) {
  setLineWrapMode(enable? WidgetWidth : NoWrap);
  updateSizeHint();
}

void MultiLineEdit::setPasteProtectionEnabled(bool enable, QWidget *) {
  _pasteProtectionEnabled = enable;
}

void MultiLineEdit::historyMoveBack() {
  addToHistory(text(), true);

  if(idx > 0) {
    idx--;
    showHistoryEntry();
  }
}

void MultiLineEdit::historyMoveForward() {
  addToHistory(text(), true);

  if(idx < history.count()) {
    idx++;
    if(idx < history.count() || tempHistory.contains(idx)) // tempHistory might have an entry for idx == history.count() + 1
      showHistoryEntry();
    else
      reset();              // equals clear() in this case
  } else {
    addToHistory(text());
    reset();
  }
}

bool MultiLineEdit::addToHistory(const QString &text, bool temporary) {
  if(text.isEmpty())
    return false;

  Q_ASSERT(0 <= idx && idx <= history.count());

  if(temporary) {
    // if an entry of the history is changed, we remember it and show it again at this
    // position until a line was actually sent
    // sent lines get appended to the history
    if(history.isEmpty() || text != history[idx - (int)(idx == history.count())]) {
      tempHistory[idx] = text;
      return true;
    }
  } else {
    if(history.isEmpty() || text != history.last()) {
      history << text;
      tempHistory.clear();
      return true;
    }
  }
  return false;
}

void MultiLineEdit::keyPressEvent(QKeyEvent *event) {
  // Workaround the fact that Qt < 4.5 doesn't know InsertLineSeparator yet
#if QT_VERSION >= 0x040500
  if(event == QKeySequence::InsertLineSeparator) {
#else

# ifdef Q_WS_MAC
  if((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) && event->modifiers() & Qt::META) {
# else
  if((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) && event->modifiers() & Qt::SHIFT) {
# endif
#endif

    if(_mode == SingleLine)
      return;
#ifdef HAVE_KDE
    KTextEdit::keyPressEvent(event);
#else
    QTextEdit::keyPressEvent(event);
#endif
    return;
  }

  switch(event->key()) {
  case Qt::Key_Up:
    if(event->modifiers() & Qt::ShiftModifier)
      break;
    {
      event->accept();
      if(!(event->modifiers() & Qt::ControlModifier)) {
        int pos = textCursor().position();
        moveCursor(QTextCursor::Up);
        if(pos == textCursor().position()) // already on top line -> history
          historyMoveBack();
      } else
        historyMoveBack();
      return;
    }

  case Qt::Key_Down:
    if(event->modifiers() & Qt::ShiftModifier)
      break;
    {
      event->accept();
      if(!(event->modifiers() & Qt::ControlModifier)) {
        int pos = textCursor().position();
        moveCursor(QTextCursor::Down);
        if(pos == textCursor().position()) // already on bottom line -> history
          historyMoveForward();
      } else
        historyMoveForward();
      return;
    }

  case Qt::Key_Return:
  case Qt::Key_Enter:
  case Qt::Key_Select:
    event->accept();
    on_returnPressed();
    return;

  // We don't want to have the tab key react even if no completer is installed
  case Qt::Key_Tab:
    event->accept();
    return;

  default:
    ;
  }


#ifdef HAVE_KDE
  KTextEdit::keyPressEvent(event);
#else
  QTextEdit::keyPressEvent(event);
#endif
}

void MultiLineEdit::on_returnPressed() {
  on_returnPressed(text());
}

void MultiLineEdit::on_returnPressed(const QString & text) {
  if(!text.isEmpty()) {
    foreach(const QString &line, text.split('\n', QString::SkipEmptyParts)) {
      if(line.isEmpty())
        continue;
      addToHistory(line);
      emit textEntered(line);
    }
    reset();
    tempHistory.clear();
  }
}

void MultiLineEdit::on_textChanged() {
  QString newText = text();
  newText.replace("\r\n", "\n");
  newText.replace('\r', '\n');
  if(_mode == SingleLine) {
    if(!pasteProtectionEnabled())
      newText.replace('\n', ' ');
    else if(newText.contains('\n')) {
      QStringList lines = newText.split('\n', QString::SkipEmptyParts);
      clear();

      if(lines.count() >= 4) {
        QString msg = tr("Do you really want to paste %n lines?", "", lines.count());
        msg += "<p>";
        for(int i = 0; i < 4; i++) {
          msg += Qt::escape(lines[i].left(40));
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
        if(question.exec() != QMessageBox::Yes)
          return;
      }

      foreach(QString line, lines) {
        clear();
        insert(line);
        on_returnPressed();
      }
    }
  }

  _singleLine = (newText.indexOf('\n') < 0);

  if(document()->size().height() != _lastDocumentHeight) {
    _lastDocumentHeight = document()->size().height();
    on_documentHeightChanged(_lastDocumentHeight);
  }
  updateSizeHint();
}

void MultiLineEdit::on_documentHeightChanged(qreal) {
  updateScrollBars();
}

void MultiLineEdit::reset() {
  // every time the MultiLineEdit is cleared we also reset history index
  idx = history.count();
  clear();
  QTextBlockFormat format = textCursor().blockFormat();
  format.setLeftMargin(leftMargin); // we want a little space between the frame and the contents
  textCursor().setBlockFormat(format);
  updateScrollBars();
}

void MultiLineEdit::showHistoryEntry() {
  // if the user changed the history, display the changed line
  setPlainText(tempHistory.contains(idx) ? tempHistory[idx] : history[idx]);
  QTextCursor cursor = textCursor();
  QTextBlockFormat format = cursor.blockFormat();
  format.setLeftMargin(leftMargin); // we want a little space between the frame and the contents
  cursor.setBlockFormat(format);
  cursor.movePosition(QTextCursor::End);
  setTextCursor(cursor);
  updateScrollBars();
}
