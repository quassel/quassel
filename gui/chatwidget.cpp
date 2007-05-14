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

#include "util.h"
#include "style.h"
#include "chatwidget.h"
#include <QtGui>
#include <QtCore>

ChatWidget::ChatWidget(QWidget *parent) : QAbstractScrollArea(parent) {
  scrollTimer = new QTimer(this);
  scrollTimer->setSingleShot(false);
  scrollTimer->setInterval(100);
  setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  setMinimumSize(QSize(400,400));
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  bottomLine = -1;
  height = 0;
  ycoords.append(0);
  pointerPosition = QPoint(0,0);
  connect(verticalScrollBar(), SIGNAL(actionTriggered(int)), this, SLOT(scrollBarAction(int)));
  connect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(scrollBarValChanged(int)));
}

void ChatWidget::init(QString netname, QString bufname) {
  networkName = netname;
  bufferName = bufname;
  setBackgroundRole(QPalette::Base);
  setFont(QFont("Fixed"));
  tsWidth = 90;
  senderWidth = 100;
  computePositions();
  adjustScrollBar();
  verticalScrollBar()->setValue(verticalScrollBar()->maximum());
  //verticalScrollBar()->setPageStep(viewport()->height());
  //verticalScrollBar()->setSingleStep(20);
  //verticalScrollBar()->setMinimum(0);
  //verticalScrollBar()->setMaximum((int)height - verticalScrollBar()->pageStep());

  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  setMouseTracking(true);
  mouseMode = Normal;
  selectionMode = NoSelection;
  connect(scrollTimer, SIGNAL(timeout()), this, SLOT(handleScrollTimer()));
}

ChatWidget::~ChatWidget() {
  //qDebug() << "destroying chatwidget" << bufferName;
  //foreach(ChatLine *l, lines) {
  //  delete l;
  //}
}

QSize ChatWidget::sizeHint() const {
  //qDebug() << size();
  return size();
}

void ChatWidget::adjustScrollBar() {
  verticalScrollBar()->setPageStep(viewport()->height());
  verticalScrollBar()->setSingleStep(20);
  verticalScrollBar()->setMinimum(0);
  verticalScrollBar()->setMaximum((int)height - verticalScrollBar()->pageStep());
  //qDebug() << height << viewport()->height() << verticalScrollBar()->pageStep();
  //if(bottomLine < 0) {
  //  verticalScrollBar()->setValue(verticalScrollBar()->maximum());
  //} else {
    //int bot = verticalScrollBar()->value() + viewport()->height(); //qDebug() << bottomLine;
    //verticalScrollBar()->setValue(qMax(0, (int)ycoords[bottomLine+1] - viewport()->height()));
  //}
}

void ChatWidget::scrollBarValChanged(int val) {
  return;
  if(val >= verticalScrollBar()->maximum()) bottomLine = -1;
  else {
    int bot = val + viewport()->height();
    int line = yToLineIdx(bot);
    //bottomLine = line;
  }
}

void ChatWidget::scrollBarAction(int action) {
  switch(action) {
    case QScrollBar::SliderSingleStepAdd:
      // More elaborate. But what with loooong lines?
      // verticalScrollBar()->setValue((int)ycoords[yToLineIdx(verticalScrollBar()->value() + viewport()->height()) + 1] - viewport()->height());
      break;
    case QScrollBar::SliderSingleStepSub:
      //verticalScrollBar()->setValue((int)ycoords[yToLineIdx(verticalScrollBar()->value())]);
      break;

  }

}

void ChatWidget::handleScrollTimer() {
  if(mouseMode == MarkText || mouseMode == MarkLines) {
    if(pointerPosition.y() > viewport()->height()) {
      verticalScrollBar()->setValue(verticalScrollBar()->value() + pointerPosition.y() - viewport()->height());
      handleMouseMoveEvent(QPoint(pointerPosition.x(), viewport()->height()));
    } else if(pointerPosition.y() < 0) {
      verticalScrollBar()->setValue(verticalScrollBar()->value() + pointerPosition.y());
      handleMouseMoveEvent(QPoint(pointerPosition.x(), 0));
    }
  }
}

void ChatWidget::ensureVisible(int line) {
  int top = verticalScrollBar()->value();
  int bot = top + viewport()->height();
  if(ycoords[line+1] > bot) {
    verticalScrollBar()->setValue(qMax(0, (int)ycoords[line+1] - viewport()->height()));
  } else if(ycoords[line] < top) {
    verticalScrollBar()->setValue((int)ycoords[line]);
  }

}

void ChatWidget::clear() {
  //contents->clear();
}

void ChatWidget::prependChatLine(ChatLine *line) {
  qreal h = line->layout(tsWidth, senderWidth, textWidth);
  for(int i = 1; i < ycoords.count(); i++) ycoords[i] += h;
  ycoords.insert(1, h);
  lines.prepend(line);
  height += h;
  // Fix all variables containing line numbers
  dragStartLine ++;
  curLine ++;
  selectionStart ++; selectionEnd ++;
  adjustScrollBar();
  verticalScrollBar()->setValue(verticalScrollBar()->value() + (int)h);
  viewport()->update();
}

void ChatWidget::prependChatLines(QList<ChatLine *> clist) {
  QList<qreal> tmpy; tmpy.append(0);
  qreal h = 0;
  foreach(ChatLine *l, clist) {
    h += l->layout(tsWidth, senderWidth, textWidth);
    tmpy.append(h);
  }
  ycoords.removeFirst();
  for(int i = 0; i < ycoords.count(); i++) ycoords[i] += h;
  ycoords = tmpy + ycoords;
  lines = clist + lines;
  height += h;
  // Fix all variables containing line numbers
  int i = clist.count();
  dragStartLine += i;
  curLine += i;
  selectionStart += i; selectionEnd += i; //? selectionEnd += i;
  //if(bottomLine >= 0) bottomLine += i;
  adjustScrollBar();
  //verticalScrollBar()->setPageStep(viewport()->height());
  //verticalScrollBar()->setSingleStep(20);
  //verticalScrollBar()->setMaximum((int)height - verticalScrollBar()->pageStep());
  verticalScrollBar()->setValue(verticalScrollBar()->value() + (int)h);
  viewport()->update();
}


void ChatWidget::appendChatLine(ChatLine *line) {
  qreal h = line->layout(tsWidth, senderWidth, textWidth);
  ycoords.append(h + ycoords[ycoords.count() - 1]);
  height += h;
  bool flg = (verticalScrollBar()->value() == verticalScrollBar()->maximum());
  adjustScrollBar();
  if(flg) verticalScrollBar()->setValue(verticalScrollBar()->maximum());
  lines.append(line);
  viewport()->update();
}

void ChatWidget::appendChatLines(QList<ChatLine *> list) {
  foreach(ChatLine *line, list) {
    qreal h = line->layout(tsWidth, senderWidth, textWidth);
    ycoords.append(h + ycoords[ycoords.count() - 1]);
    height += h;
    lines.append(line);
  }
  bool flg = (verticalScrollBar()->value() == verticalScrollBar()->maximum());
  adjustScrollBar();
  if(flg) verticalScrollBar()->setValue(verticalScrollBar()->maximum());
  viewport()->update();
}

void ChatWidget::setContents(QList<ChatLine *> list) {
  ycoords.clear();
  ycoords.append(0);
  height = 0;
  lines.clear();
  appendChatLines(list);
}

//!\brief Computes the different x position vars for given tsWidth and senderWidth.
void ChatWidget::computePositions() {
  senderX = tsWidth + Style::sepTsSender();
  textX = senderX + senderWidth + Style::sepSenderText();
  tsGrabPos = tsWidth + (int)Style::sepTsSender()/2;
  senderGrabPos = senderX + senderWidth + (int)Style::sepSenderText()/2;
  textWidth = viewport()->size().width() - textX;
}

void ChatWidget::resizeEvent(QResizeEvent *event) {
  //qDebug() << bufferName << isVisible() << event->size() << event->oldSize();
  /*if(event->oldSize().isValid())*/
  //contents->setWidth(event->size().width());
  //setAlignment(Qt::AlignBottom);
  if(event->size().width() != event->oldSize().width()) {
    computePositions();
    layout();
  }
  //adjustScrollBar();
  //qDebug() << viewport()->size() << viewport()->height();
  //QAbstractScrollArea::resizeEvent(event);
  //qDebug() << viewport()->size() << viewport()->geometry();
}

void ChatWidget::paintEvent(QPaintEvent *event) {
  QPainter painter(viewport());

  //qDebug() <<  verticalScrollBar()->value();
  painter.translate(0, -verticalScrollBar()->value());
  int top = event->rect().top() + verticalScrollBar()->value();
  int bot = top + event->rect().height();
  int idx = yToLineIdx(top);
  if(idx < 0) return;
  for(int i = idx; i < lines.count() ; i++) {
    lines[i]->draw(&painter, QPointF(0, ycoords[i]));
    if(ycoords[i+1] > bot) return;
  }
}

//!\brief Layout the widget.
void ChatWidget::layout() {
  // TODO fix scrollbars
  //int botLine = yToLineIdx(verticalScrollBar()->value() + 
  qreal y = 0;
  for(int i = 0; i < lines.count(); i++) {
    qreal h = lines[i]->layout(tsWidth, senderWidth, textWidth);
    ycoords[i+1] = h + ycoords[i];
  }
  height = ycoords[ycoords.count()-1];
  adjustScrollBar();
  verticalScrollBar()->setValue(verticalScrollBar()->maximum());
  viewport()->update();
}

int ChatWidget::yToLineIdx(qreal y) {
  if(y >= ycoords[ycoords.count()-1]) ycoords.count()-1;
  if(ycoords.count() <= 1) return 0;
  int uidx = 0;
  int oidx = ycoords.count() - 1;
  int idx;
  while(1) {
    if(uidx == oidx - 1) return uidx;
    idx = (uidx + oidx) / 2;
    if(ycoords[idx] > y) oidx = idx;
    else uidx = idx;
  }
}

void ChatWidget::mousePressEvent(QMouseEvent *event) {
  if(lines.isEmpty()) return;
  QPoint pos = event->pos() + QPoint(0, verticalScrollBar()->value());
  if(event->button() == Qt::LeftButton) {
    dragStartPos = pos;
    dragStartMode = Normal;
    switch(mouseMode) {
      case Normal:
        if(mousePos == OverTsSep) {
          dragStartMode = DragTsSep;
          setCursor(Qt::ClosedHandCursor);
        } else if(mousePos == OverTextSep) {
          dragStartMode = DragTextSep;
          setCursor(Qt::ClosedHandCursor);
        } else {
          dragStartLine = yToLineIdx(pos.y());
          dragStartCursor = lines[dragStartLine]->posToCursor(QPointF(pos.x(), pos.y()-ycoords[dragStartLine]));
        }
        mouseMode = Pressed;
        break;
    }
  }
}

void ChatWidget::mouseDoubleClickEvent(QMouseEvent *event) {



}

void ChatWidget::mouseReleaseEvent(QMouseEvent *event) {
  //QPoint pos = event->pos() + QPoint(0, verticalScrollBar()->value());

  if(event->button() == Qt::LeftButton) {
    dragStartPos = QPoint();
    if(mousePos == OverTsSep || mousePos == OverTextSep) setCursor(Qt::OpenHandCursor);
    else setCursor(Qt::ArrowCursor);

    switch(mouseMode) {
      case Pressed:
        mouseMode = Normal;
        clearSelection();
        break;
      case MarkText:
        mouseMode = Normal;
        selectionMode = TextSelected;
        selectionLine = dragStartLine;
        selectionStart = qMin(dragStartCursor, curCursor);
        selectionEnd = qMax(dragStartCursor, curCursor);
        // TODO Make X11SelectionMode configurable!
        QApplication::clipboard()->setText(selectionToString());
        break;
      case MarkLines:
        mouseMode = Normal;
        selectionMode = LinesSelected;
        selectionStart = qMin(dragStartLine, curLine);
        selectionEnd = qMax(dragStartLine, curLine);
        // TODO Make X11SelectionMode configurable!
        QApplication::clipboard()->setText(selectionToString());
        break;
      default:
        mouseMode = Normal;
    }
  }
}

//!\brief React to mouse movements over the ChatWidget.
/** This is called by Qt whenever the mouse moves. Here we have to do most of the mouse handling,
 * such as changing column widths, marking text or initiating drag & drop.
 */
void ChatWidget::mouseMoveEvent(QMouseEvent *event) {
  QPoint pos = event->pos(); pointerPosition = pos;
  // Scroll if mouse pointer leaves widget while dragging
  if((mouseMode == MarkText || mouseMode == MarkLines) && (pos.y() > viewport()->height() || pos.y() < 0)) {
    if(!scrollTimer->isActive()) {
      scrollTimer->start();
    }
  } else {
    if(scrollTimer->isActive()) {
      scrollTimer->stop();
    }
  }
  handleMouseMoveEvent(pos);
}

void ChatWidget::handleMouseMoveEvent(const QPoint &_pos) {
  // FIXME
  if(lines.count() <= 0) return;
  // Set some basic properties of the current position
  QPoint pos = _pos + QPoint(0, verticalScrollBar()->value());
  int x = pos.x();
  int y = pos.y();
  MousePos oldpos = mousePos;
  if(x >= tsGrabPos - 3 && x <= tsGrabPos + 3) mousePos = OverTsSep;
  else if(x >= senderGrabPos - 3 && x <= senderGrabPos + 3) mousePos = OverTextSep;
  else mousePos = None;

  // Pass 1: Do whatever we can before switching mouse mode (if at all).
  switch(mouseMode) {
    // No special mode. Set mouse cursor if appropriate.
    case Normal:
    {
      //if(oldpos != mousePos) {
      if(mousePos == OverTsSep || mousePos == OverTextSep) setCursor(Qt::OpenHandCursor);
      else {
        int l = yToLineIdx(y);
        int c = lines[l]->posToCursor(QPointF(x, y - ycoords[l]));
        if(c >= 0 && lines[l]->isUrl(c)) {
          setCursor(Qt::PointingHandCursor);
        } else {
          setCursor(Qt::ArrowCursor);
        }
      }
    }
      break;
    // Left button pressed. Might initiate marking or drag & drop if we moved past the drag distance.
    case Pressed:
      if(!dragStartPos.isNull() && (dragStartPos - pos).manhattanLength() >= QApplication::startDragDistance()) {
        // Moving a column separator?
        if(dragStartMode == DragTsSep) mouseMode = DragTsSep;
        else if(dragStartMode == DragTextSep) mouseMode = DragTextSep;
        // Nope. Check if we are over a selection to start drag & drop.
        else if(dragStartMode == Normal) {
          bool dragdrop = false;
          if(selectionMode == TextSelected) {
            int l = yToLineIdx(y);
            if(selectionLine == l) {
              int p = lines[l]->posToCursor(QPointF(x, y - ycoords[l]));
              if(p >= selectionStart && p <= selectionEnd) dragdrop = true;
            }
          } else if(selectionMode == LinesSelected) {
            int l = yToLineIdx(y);
            if(l >= selectionStart && l <= selectionEnd) dragdrop = true;
          }
          // Ok, so just start drag & drop if appropriate.
          if(dragdrop) {
            QDrag *drag = new QDrag(this);
            QMimeData *mimeData = new QMimeData;
            mimeData->setText(selectionToString());
            drag->setMimeData(mimeData);
            drag->start();
            mouseMode = Normal;
          // Otherwise, clear the selection and start text marking!
          } else {
            setCursor(Qt::ArrowCursor);
            clearSelection();
            if(dragStartCursor < 0) { mouseMode = MarkLines; curLine = -1; }
            else mouseMode = MarkText;
          }
        }
      }
      break;
    case DragTsSep:
      break;
    case DragTextSep:
      break;
  }
  // Pass 2: Some mouse modes need work after being set...
  if(mouseMode == DragTsSep && x < size().width() - Style::sepSenderText() - senderWidth - 10) {
    // Drag first column separator
    int foo = Style::sepTsSender()/2;
    tsWidth = qMax(x, foo) - foo;
    computePositions();
    layout();
  } else if(mouseMode == DragTextSep && x < size().width() - 10) {
    // Drag second column separator
    int foo = tsWidth + Style::sepTsSender() + Style::sepSenderText()/2;
    senderWidth = qMax(x, foo) - foo;
    computePositions();
    layout();
  } else if(mouseMode == MarkText) {
    // Change currently marked text
    curLine = yToLineIdx(y);
    int c = lines[curLine]->posToCursor(QPointF(x, y - ycoords[curLine]));
    if(curLine == dragStartLine && c >= 0) {
      if(c != curCursor) {
        curCursor = c;
        lines[curLine]->setSelection(ChatLine::Partial, dragStartCursor, c);
        viewport()->update();
      }
    } else {
      mouseMode = MarkLines;
      selectionStart = qMin(curLine, dragStartLine); selectionEnd = qMax(curLine, dragStartLine);
      for(int i = selectionStart; i <= selectionEnd; i++) lines[i]->setSelection(ChatLine::Full);
      viewport()->update();
    }
  } else if(mouseMode == MarkLines) {
    // Line marking
    int l = yToLineIdx(y);
    if(l != curLine) {
      selectionStart = qMin(l, dragStartLine); selectionEnd = qMax(l, dragStartLine);
      if(curLine < 0) {
        Q_ASSERT(selectionStart == selectionEnd);
        lines[l]->setSelection(ChatLine::Full);
      } else {
        if(curLine < selectionStart) {
          for(int i = curLine; i < selectionStart; i++) lines[i]->setSelection(ChatLine::None);
        } else if(curLine > selectionEnd) {
          for(int i = selectionEnd+1; i <= curLine; i++) lines[i]->setSelection(ChatLine::None);
        } else if(selectionStart < curLine && l < curLine) {
          for(int i = selectionStart; i < curLine; i++) lines[i]->setSelection(ChatLine::Full);
        } else if(curLine < selectionEnd && l > curLine) {
          for(int i = curLine+1; i <= selectionEnd; i++) lines[i]->setSelection(ChatLine::Full);
        }
      }
      curLine = l;
      //ensureVisible(l);
      viewport()->update();
    }
  }
}

//!\brief Clear current text selection.
void ChatWidget::clearSelection() {
  if(selectionMode == TextSelected) {
    lines[selectionLine]->setSelection(ChatLine::None);
  } else if(selectionMode == LinesSelected) {
    for(int i = selectionStart; i <= selectionEnd; i++) {
      lines[i]->setSelection(ChatLine::None);
    }
  }
  selectionMode = NoSelection;
  viewport()->update();
}

//!\brief Convert current selection to human-readable string.
QString ChatWidget::selectionToString() {
  //TODO Make selection format configurable!
  if(selectionMode == NoSelection) return "";
  if(selectionMode == LinesSelected) {
    QString result;
    for(int l = selectionStart; l <= selectionEnd; l++) {
      result += QString("[%1] %2 %3\n").arg(lines[l]->timeStamp().toLocalTime().toString("hh:mm:ss"))
          .arg(lines[l]->sender()).arg(lines[l]->text());
    }
    return result;
  }
  // selectionMode == TextSelected
  return lines[selectionLine]->text().mid(selectionStart, selectionEnd - selectionStart);
}

/************************************************************************************/

//!\brief Construct a ChatLine object from a message.
/**
 * \param m   The message to be layouted and rendered
 * \param net The network name
 * \param buf The buffer name
 */ 
ChatLine::ChatLine(Message m) : QObject() {
  hght = 0;
  //networkName = m.buffer.network();
  //bufferName = m.buffer.buffer();
  msg = m;
  selectionMode = None;
  formatMsg(msg);
}

ChatLine::~ChatLine() {

}

void ChatLine::formatMsg(Message msg) {
  QString user = userFromMask(msg.sender);
  QString host = hostFromMask(msg.sender);
  QString nick = nickFromMask(msg.sender);
  QString text = Style::mircToInternal(msg.text);
  QString networkName = msg.buffer.network();
  QString bufferName = msg.buffer.buffer();

  QString c = tr("%DT[%1]").arg(msg.timeStamp.toLocalTime().toString("hh:mm:ss"));
  QString s, t;
  switch(msg.type) {
    case Message::Plain:
      s = tr("%DS<%1>").arg(nick); t = tr("%D0%1").arg(text); break;
    case Message::Server:
      s = tr("%Ds*"); t = tr("%Ds%1").arg(text); break;
    case Message::Error:
      s = tr("%De*"); t = tr("%De%1").arg(text); break;
    case Message::Join:
      s = tr("%Dj-->"); t = tr("%Dj%DN%DU%1%DU%DN %DH(%2@%3)%DH has joined %DC%DU%4%DU%DC").arg(nick, user, host, bufferName); break;
    case Message::Part:
      s = tr("%Dp<--"); t = tr("%Dp%DN%DU%1%DU%DN %DH(%2@%3)%DH has left %DC%DU%4%DU%DC").arg(nick, user, host, bufferName);
      if(!text.isEmpty()) t = QString("%1 (%2)").arg(t).arg(text);
      break;
    case Message::Quit:
      s = tr("%Dq<--"); t = tr("%Dq%DN%DU%1%DU%DN %DH(%2@%3)%DH has quit").arg(nick, user, host);
      if(!text.isEmpty()) t = QString("%1 (%2)").arg(t).arg(text);
      break;
    case Message::Kick:
      { s = tr("%Dk<-*");
        QString victim = text.section(" ", 0, 0);
        //if(victim == ui.ownNick->currentText()) victim = tr("you");
        QString kickmsg = text.section(" ", 1);
        t = tr("%Dk%DN%DU%1%DU%DN has kicked %DN%DU%2%DU%DN from %DC%DU%3%DU%DC").arg(nick).arg(victim).arg(bufferName);
        if(!kickmsg.isEmpty()) t = QString("%1 (%2)").arg(t).arg(kickmsg);
      }
      break;
    case Message::Nick:
      s = tr("%Dr<->");
      if(nick == msg.text) t = tr("%DrYou are now known as %DN%1%DN").arg(msg.text);
      else t = tr("%Dr%DN%1%DN is now known as %DN%DU%2%DU%DN").arg(nick, msg.text);
      break;
    case Message::Mode:
      s = tr("%Dm***");
      if(nick.isEmpty()) t = tr("%DmUser mode: %DM%1%DM").arg(msg.text);
      else t = tr("%DmMode %DM%1%DM by %DN%DU%2%DU%DN").arg(msg.text, nick);
      break;
    default:
      s = tr("%De%1").arg(msg.sender);
      t = tr("%De[%1]").arg(msg.text);
  }
  QTextOption tsOption, senderOption, textOption;
  tsFormatted = Style::internalToFormatted(c);
  senderFormatted = Style::internalToFormatted(s);
  textFormatted = Style::internalToFormatted(t);
  precomputeLine();
}

QList<ChatLine::FormatRange> ChatLine::calcFormatRanges(const Style::FormattedString &fs, QTextLayout::FormatRange additional) {
  QList<FormatRange> ranges;
  QList<QTextLayout::FormatRange> formats = fs.formats;
  formats.append(additional);
  int cur = -1;
  FormatRange range, lastrange;
  for(int i = 0; i < fs.text.length(); i++) {
    QTextCharFormat format;
    foreach(QTextLayout::FormatRange f, formats) {
      if(i >= f.start && i < f.start + f.length) format.merge(f.format);
    }
    if(cur < 0) {
      range.start = 0; range.length = 1; range.format= format;
      cur = 0;
    } else {
      if(format == range.format) range.length++;
      else {
        QFontMetrics metrics(range.format.font());
        range.height = metrics.lineSpacing();
        ranges.append(range);
        range.start = i; range.length = 1; range.format = format;
        cur++;
      }
    }
  }
  if(cur >= 0) {
    QFontMetrics metrics(range.format.font());
    range.height = metrics.lineSpacing();
    ranges.append(range);
  }
  return ranges;
}

void ChatLine::setSelection(SelectionMode mode, int start, int end) {
  selectionMode = mode;
  //tsFormat.clear(); senderFormat.clear(); textFormat.clear();
  QPalette pal = QApplication::palette();
  QTextLayout::FormatRange tsSel, senderSel, textSel;
  switch (mode) {
    case None:
      tsFormat = calcFormatRanges(tsFormatted);
      senderFormat = calcFormatRanges(senderFormatted);
      textFormat = calcFormatRanges(textFormatted);
      break;
    case Partial:
      selectionStart = qMin(start, end); selectionEnd = qMax(start, end);
      textSel.format.setForeground(pal.brush(QPalette::HighlightedText));
      textSel.format.setBackground(pal.brush(QPalette::Highlight));
      textSel.start = selectionStart;
      textSel.length = selectionEnd - selectionStart;
      //textFormat.append(textSel);
      textFormat = calcFormatRanges(textFormatted, textSel);
      foreach(FormatRange fr, textFormat);
      break;
    case Full:
      tsSel.format.setForeground(pal.brush(QPalette::HighlightedText));
      tsSel.format.setBackground(pal.brush(QPalette::Highlight));
      tsSel.start = 0; tsSel.length = tsFormatted.text.length();
      tsFormat = calcFormatRanges(tsFormatted, tsSel);
      senderSel.format.setForeground(pal.brush(QPalette::HighlightedText));
      senderSel.format.setBackground(pal.brush(QPalette::Highlight));
      senderSel.start = 0; senderSel.length = senderFormatted.text.length();
      senderFormat = calcFormatRanges(senderFormatted, senderSel);
      textSel.format.setForeground(pal.brush(QPalette::HighlightedText));
      textSel.format.setBackground(pal.brush(QPalette::Highlight));
      textSel.start = 0; textSel.length = textFormatted.text.length();
      textFormat = calcFormatRanges(textFormatted, textSel);
      break;
  }
}

uint ChatLine::msgId() {
  return msg.buffer.uid();
}

BufferId ChatLine::bufferId() {
  return msg.buffer;
}

QDateTime ChatLine::timeStamp() {
  return msg.timeStamp;
}

QString ChatLine::sender() {
  return senderFormatted.text;
}

QString ChatLine::text() {
  return textFormatted.text;
}

bool ChatLine::isUrl(int c) {
  if(c < 0 || c >= charUrlIdx.count()) return false;;
  return charUrlIdx[c] >= 0;
}

QUrl ChatLine::getUrl(int c) {
  if(c < 0 || c >= charUrlIdx.count()) return QUrl();
  int i = charUrlIdx[c];
  if(i >= 0) return textFormatted.urls[i].url;
  else return QUrl();
}

//!\brief Return the cursor position for the given coordinate pos.
/**
 * \param pos The position relative to the ChatLine
 * \return The cursor position, [or -3 for invalid,] or -2 for timestamp, or -1 for sender
 */
int ChatLine::posToCursor(QPointF pos) {
  if(pos.x() < tsWidth + (int)Style::sepTsSender()/2) return -2;
  qreal textStart = tsWidth + Style::sepTsSender() + senderWidth + Style::sepSenderText();
  if(pos.x() < textStart) return -1;
  int x = (int)(pos.x() - textStart);
  for(int l = lineLayouts.count() - 1; l >=0; l--) {
    LineLayout line = lineLayouts[l];
    if(pos.y() >= line.y) {
      int offset = charPos[line.start]; x += offset;
      for(int i = line.start + line.length - 1; i >= line.start; i--) {
        if((charPos[i] + charPos[i+1])/2 <= x) return i+1; // FIXME: Optimize this!
      }
      return line.start;
    }
  }
}

void ChatLine::precomputeLine() {
  tsFormat = calcFormatRanges(tsFormatted);
  senderFormat = calcFormatRanges(senderFormatted);
  textFormat = calcFormatRanges(textFormatted);

  minHeight = 0;
  foreach(FormatRange fr, tsFormat) minHeight = qMax(minHeight, fr.height);
  foreach(FormatRange fr, senderFormat) minHeight = qMax(minHeight, fr.height);

  words.clear();
  charPos.resize(textFormatted.text.length() + 1);
  charHeights.resize(textFormatted.text.length());
  charUrlIdx.fill(-1, textFormatted.text.length());
  for(int i = 0; i < textFormatted.urls.count(); i++) {
    Style::UrlInfo url = textFormatted.urls[i];
    for(int j = url.start; j < url.end; j++) charUrlIdx[j] = i;
  }
  if(!textFormat.count()) return;
  int idx = 0; int cnt = 0; int w = 0; int h = 0;
  QFontMetrics metrics(textFormat[0].format.font());
  Word wr;
  wr.start = -1; wr.trailing = -1;
  for(int i = 0; i < textFormatted.text.length(); ) {
    charPos[i] = w; charHeights[i] = textFormat[idx].height;
    w += metrics.charWidth(textFormatted.text, i);
    if(!textFormatted.text[i].isSpace()) {
      if(wr.trailing >= 0) {
        // new word after space
        words.append(wr);
        wr.start = -1;
      }
      if(wr.start < 0) {
        wr.start = i; wr.length = 1; wr.trailing = -1; wr.height = textFormat[idx].height;
      } else {
        wr.length++; wr.height = qMax(wr.height, textFormat[idx].height);
      }
    } else {
      if(wr.start < 0) {
        wr.start = i; wr.length = 0; wr.trailing = 1; wr.height = 0;
      } else {
        wr.trailing++;
      }
    }
    if(++i < textFormatted.text.length() && ++cnt >= textFormat[idx].length) {
      cnt = 0; idx++;
      Q_ASSERT(idx < textFormat.count());
      metrics = QFontMetrics(textFormat[idx].format.font());
    }
  }
  charPos[textFormatted.text.length()] = w;
  if(wr.start >= 0) words.append(wr);
}

qreal ChatLine::layout(qreal tsw, qreal senderw, qreal textw) {
  tsWidth = tsw; senderWidth = senderw; textWidth = textw;
  if(textw <= 0) return minHeight;
  lineLayouts.clear(); LineLayout line;
  int h = 0;
  int offset = 0; int numWords = 0;
  line.y = 0;
  line.start = 0;
  line.height = minHeight;  // first line needs room for ts and sender
  for(int i = 0; i < words.count(); i++) {
    int lastpos = charPos[words[i].start + words[i].length]; // We use charPos[lastchar + 1], 'coz last char needs to fit
    if(lastpos - offset <= textw) {
      line.height = qMax(line.height, words[i].height);
      line.length = words[i].start + words[i].length - line.start;
      numWords++;
    } else {
      // we need to wrap!
      if(numWords > 0) {
        // ok, we had some words before, so store the layout and start a new line
        h += line.height;
        line.length = words[i-1].start + words[i-1].length - line.start;
        lineLayouts.append(line);
        line.y += line.height;
        line.start = words[i].start;
        line.height = words[i].height;
        offset = charPos[words[i].start];
      }
      numWords = 1;
      // check if the word fits into the current line
      if(lastpos - offset <= textw) {
        line.length = words[i].length;
      } else {
        // we need to break a word in the middle
        int border = (int)textw + offset; // save some additions
        line.start = words[i].start;
        line.length = 1;
        line.height = charHeights[line.start];
        int j = line.start + 1;
        for(int l = 1; l < words[i].length; j++, l++) {
          if(charPos[j+1] < border) {
            line.length++;
            line.height = qMax(line.height, charHeights[j]);
            continue;
          } else {
            h += line.height;
            lineLayouts.append(line);
            line.y += line.height;
            line.start = j;
            line.height = charHeights[j];
            line.length = 1;
            offset = charPos[j];
            border = (int)textw + offset;
          }
        }
      }
    }
  }
  h += line.height;
  if(numWords > 0) {
    lineLayouts.append(line);
  }
  hght = h;
  return hght;
}

//!\brief Draw ChatLine on the given QPainter at the given position.
void ChatLine::draw(QPainter *p, const QPointF &pos) {
  QPalette pal = QApplication::palette();

  if(selectionMode == Full) {
    p->setPen(Qt::NoPen);
    p->setBrush(pal.brush(QPalette::Highlight));
    p->drawRect(QRectF(pos, QSizeF(tsWidth + Style::sepTsSender() + senderWidth + Style::sepSenderText() + textWidth, height())));
  } else if(selectionMode == Partial) {

  } /*
  p->setClipRect(QRectF(pos, QSizeF(tsWidth, height())));
  tsLayout.draw(p, pos, tsFormat);
  p->setClipRect(QRectF(pos + QPointF(tsWidth + Style::sepTsSender(), 0), QSizeF(senderWidth, height())));
  senderLayout.draw(p, pos + QPointF(tsWidth + Style::sepTsSender(), 0), senderFormat);
  p->setClipping(false);
  textLayout.draw(p, pos + QPointF(tsWidth + Style::sepTsSender() + senderWidth + Style::sepSenderText(), 0), textFormat);
  */
  //p->setClipRect(QRectF(pos, QSizeF(tsWidth, 15)));
  //p->drawRect(QRectF(pos, QSizeF(tsWidth, minHeight)));
  p->setBackgroundMode(Qt::OpaqueMode);
  QPointF tp = pos;
  QRectF rect(pos, QSizeF(tsWidth, minHeight));
  QRectF brect;
  foreach(FormatRange fr, tsFormat) {
    p->setFont(fr.format.font());
    p->setPen(QPen(fr.format.foreground(), 0)); p->setBackground(fr.format.background());
    p->drawText(rect, Qt::AlignLeft|Qt::TextSingleLine, tsFormatted.text.mid(fr.start, fr.length), &brect);
    rect.setLeft(brect.right());
  }
  rect = QRectF(pos + QPointF(tsWidth + Style::sepTsSender(), 0), QSizeF(senderWidth, minHeight));
  for(int i = senderFormat.count() - 1; i >= 0; i--) {
    FormatRange fr = senderFormat[i];
    p->setFont(fr.format.font()); p->setPen(QPen(fr.format.foreground(), 0)); p->setBackground(fr.format.background());
    p->drawText(rect, Qt::AlignRight|Qt::TextSingleLine, senderFormatted.text.mid(fr.start, fr.length), &brect);
    rect.setRight(brect.left());
  }
  QPointF tpos = pos + QPointF(tsWidth + Style::sepTsSender() + senderWidth + Style::sepSenderText(), 0);
  qreal h = 0; int l = 0;
  rect = QRectF(tpos + QPointF(0, h), QSizeF(textWidth, lineLayouts[l].height));
  int offset = 0;
  foreach(FormatRange fr, textFormat) {
    if(l >= lineLayouts.count()) break;
    p->setFont(fr.format.font()); p->setPen(QPen(fr.format.foreground(), 0)); p->setBackground(fr.format.background());
    int start, end, frend, llend;
    do {
      frend = fr.start + fr.length;
      if(frend <= lineLayouts[l].start) break;
      llend = lineLayouts[l].start + lineLayouts[l].length;
      start = qMax(fr.start, lineLayouts[l].start); end = qMin(frend, llend);
      rect.setLeft(tpos.x() + charPos[start] - offset);
      p->drawText(rect, Qt::AlignLeft|Qt::TextSingleLine, textFormatted.text.mid(start, end - start), &brect);
      if(llend <= end) {
        h += lineLayouts[l].height;
        l++;
        if(l < lineLayouts.count()) {
          rect = QRectF(tpos + QPointF(0, h), QSizeF(textWidth, lineLayouts[l].height));
          offset = charPos[lineLayouts[l].start];
        }
      }
    } while(end < frend && l < lineLayouts.count());
  }
}

/******************************************************************************************************************/

