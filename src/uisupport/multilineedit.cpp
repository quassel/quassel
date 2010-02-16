/***************************************************************************
 *   Copyright (C) 2005-2010 by the Quassel Project                        *
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

  _mircColorMap["00"] = "#ffffff";
  _mircColorMap["01"] = "#000000";
  _mircColorMap["02"] = "#000080";
  _mircColorMap["03"] = "#008000";
  _mircColorMap["04"] = "#ff0000";
  _mircColorMap["05"] = "#800000";
  _mircColorMap["06"] = "#800080";
  _mircColorMap["07"] = "#ffa500";
  _mircColorMap["08"] = "#ffff00";
  _mircColorMap["09"] = "#00ff00";
  _mircColorMap["10"] = "#008080";
  _mircColorMap["11"] = "#00ffff";
  _mircColorMap["12"] = "#4169e1";
  _mircColorMap["13"] = "#ff00ff";
  _mircColorMap["14"] = "#808080";
  _mircColorMap["15"] = "#c0c0c0";

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
  addToHistory(convertHtmlToMircCodes(html()), true);

  if(idx > 0) {
    idx--;
    showHistoryEntry();
  }
}

void MultiLineEdit::historyMoveForward() {
  addToHistory(convertHtmlToMircCodes(html()), true);

  if(idx < history.count()) {
    idx++;
    if(idx < history.count() || tempHistory.contains(idx)) // tempHistory might have an entry for idx == history.count() + 1
      showHistoryEntry();
    else
      reset();              // equals clear() in this case
  } else {
    addToHistory(convertHtmlToMircCodes(html()));
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

    if(_mode == SingleLine) {
      event->accept();
      on_returnPressed();
      return;
    }
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

QString MultiLineEdit::convertHtmlToMircCodes(const QString &text) {
  QRegExp regexHtmlContent = QRegExp("<p.*>(.*)</p>", Qt::CaseInsensitive);

  QRegExp regexLines = QRegExp("(?:<p.*>(.*)</p>\\n?)", Qt::CaseInsensitive);
  regexLines.setMinimal(true);

  QRegExp regexStyles = QRegExp("(?:((<span.*>)(.*)</span>))", Qt::CaseInsensitive);
  regexStyles.setMinimal(true);

  QRegExp regexColors = QRegExp("((?:background-)?color):(#[0-9a-f]{6})", Qt::CaseInsensitive);
  regexStyles.setMinimal(true);

  QStringList result;
  int posLines = 0;
  QString htmlContent, pLine, line, line2, styleText, style, content;

  if (regexHtmlContent.indexIn((text)) > -1) {
    htmlContent = regexHtmlContent.cap();
    while ((posLines = regexLines.indexIn(htmlContent, posLines)) != -1) {
      pLine = regexLines.cap(1);
      QStringList lines = pLine.split("<br />");
      for (int i=0; i < lines.count(); i++) {
        line = line2 = lines[i];
        int posStyles = 0;
        while ((posStyles = regexStyles.indexIn(line2, posStyles)) != -1) {
          styleText = regexStyles.cap(1);
          style = regexStyles.cap(2);
          content = regexStyles.cap(3);

          if (style.contains("font-weight:600;")) {
            content.prepend('\x02');
            content.append('\x02');
          }
          if (style.contains("font-style:italic;")) {
            content.prepend('\x1d');
            content.append('\x1d');
          }
          if (style.contains("text-decoration: underline;")) {
            content.prepend('\x1f');
            content.append('\x1f');
          }
          if (style.contains("color:#")) { // we have either foreground or background color or both
            int posColors = 0;
            QString mircFgColor, mircBgColor;
            while ((posColors = regexColors.indexIn(style, posColors)) != -1) {
              QString colorType = regexColors.cap(1);
              QString color = regexColors.cap(2);

              if (colorType == "color")
                mircFgColor = _mircColorMap.key(color);

              if (colorType == "background-color")
                mircBgColor = _mircColorMap.key(color);

              posColors += regexColors.matchedLength();
            }
            if (!mircBgColor.isEmpty())
              content.prepend("," + mircBgColor);

            // we need a fg color to be able to use a bg color
            if (mircFgColor.isEmpty()) {
              //FIXME try to use the current forecolor
              mircFgColor = _mircColorMap.key(textColor().name());
              if (mircFgColor.isEmpty())
                mircFgColor = "01"; //use black if the current foreground color can't be converted
            }

            content.prepend(mircFgColor);
            content.prepend('\x03');
            content.append('\x03');
          }

          line.replace(styleText, content);
          posStyles += regexStyles.matchedLength();
        }

        // get rid of all remaining html tags
        QRegExp regexTags = QRegExp("<.*>",Qt::CaseInsensitive);
        regexTags.setMinimal(true);
        line.replace(regexTags, "");

        line.replace("&amp;","&");
        line.replace("&lt;","<");
        line.replace("&gt;",">");
        line.replace("&quot;","\"");

        result << line;
      }
      posLines += regexLines.matchedLength();
    }
  }
  return result.join("\n");
}

QString MultiLineEdit::convertMircCodesToHtml(const QString &text) {
  QStringList words;
  QRegExp mircCode = QRegExp("(|||)", Qt::CaseSensitive);

  int posLeft = 0;
  int posRight = 0;

  for(;;) {
    posRight = mircCode.indexIn(text, posLeft);

    if(posRight < 0) {
      words << text.mid(posLeft);
      break; // no more mirc color codes
    }

    if (posLeft < posRight) {
      words << text.mid(posLeft, posRight - posLeft);
      posLeft = posRight;
    }

    posRight = text.indexOf(mircCode.cap(), posRight + 1);
    words << text.mid(posLeft, posRight + 1 - posLeft);
    posLeft = posRight + 1;
  }

  for (int i = 0; i < words.count(); i++) {
      QString style;
      if (words[i].contains('\x02')) {
        style.append(" font-weight:600;");
        words[i].replace('\x02',"");
      }
      if (words[i].contains('\x1d')) {
        style.append(" font-style:italic;");
        words[i].replace('\x1d',"");
      }
      if (words[i].contains('\x1f')) {
        style.append(" text-decoration: underline;");
        words[i].replace('\x1f',"");
      }
      if (words[i].contains('\x03')) {
        int pos = words[i].indexOf('\x03');
        int len = 3;
        QString fg = words[i].mid(pos + 1,2);
        QString bg;
        if (words[i][pos+3] == ',')
          bg = words[i].mid(pos+4,2);

        style.append(" color:");
        style.append(_mircColorMap[fg]);
        style.append(";");

        if (!bg.isEmpty()) {
          style.append(" background-color:");
          style.append(_mircColorMap[bg]);
          style.append(";");
          len = 6;
        }
        words[i].replace(pos, len, "");
        words[i].replace('\x03',"");
      }
      words[i].replace("&","&amp;");
      words[i].replace("<", "&lt;");
      words[i].replace(">", "&gt;");
      words[i].replace("\"", "&quot;");
      if (style.isEmpty()) {
        words[i] = "<span>" + words[i] + "</span>";
      }
      else {
        words[i] = "<span style=\"" + style + "\">" + words[i] + "</span>";
      }
  }
  return words.join("");
}

void MultiLineEdit::on_returnPressed() {
  on_returnPressed(convertHtmlToMircCodes(html()));
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
  } else {
    emit noTextEntered();
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
  ensureCursorVisible();
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
  setHtml(convertMircCodesToHtml(tempHistory.contains(idx) ? tempHistory[idx] : history[idx]));
  //setPlainText(tempHistory.contains(idx) ? tempHistory[idx] : history[idx]);
  QTextCursor cursor = textCursor();
  QTextBlockFormat format = cursor.blockFormat();
  format.setLeftMargin(leftMargin); // we want a little space between the frame and the contents
  cursor.setBlockFormat(format);
  cursor.movePosition(QTextCursor::End);
  setTextCursor(cursor);
  updateScrollBars();
}
