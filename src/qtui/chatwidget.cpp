/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel Project                          *
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

#include "util.h"
#include "chatwidget.h"
#include "chatline-old.h"
#include "qtui.h"
#include "uisettings.h"

ChatWidget::ChatWidget(QWidget *parent) : QAbstractScrollArea(parent) {
  //setAutoFillBackground(false);
  //QPalette palette;
  //palette.setColor(backgroundRole(), QColor(0, 0, 0, 50));
  //setPalette(palette);
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
  UiSettings s;
  QVariant tsDef = s.value("DefaultTimestampColumnWidth", 90);
  QVariant senderDef = s.value("DefaultSenderColumnWidth", 100);
  tsWidth = s.value(QString("%1/%2/TimestampColumnWidth").arg(netname, bufname), tsDef).toInt();
  senderWidth = s.value(QString("%1/%2/SenderColumnWidth").arg(netname, bufname), senderDef).toInt();
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
  UiSettings s;
  s.setValue("DefaultTimestampColumnWidth", tsWidth);  // FIXME stupid dirty quicky
  s.setValue("DefaultSenderColumnWidth", senderWidth);
  s.setValue(QString("%1/%2/TimestampColumnWidth").arg(networkName, bufferName), tsWidth);
  s.setValue(QString("%1/%2/SenderColumnWidth").arg(networkName, bufferName), senderWidth);
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

void ChatWidget::scrollBarValChanged(int /*val*/) {
  /*
  if(val >= verticalScrollBar()->maximum()) bottomLine = -1;
  else {
    int bot = val + viewport()->height();
    int line = yToLineIdx(bot);
    //bottomLine = line;
  }
  */
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

void ChatWidget::prependMsg(AbstractUiMsg *msg) {
  ChatLine *line = dynamic_cast<ChatLine*>(msg);
  Q_ASSERT(line);
  prependChatLine(line);
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

void ChatWidget::appendMsg(AbstractUiMsg *msg) {
  ChatLine *line = dynamic_cast<ChatLine*>(msg);
  Q_ASSERT(line);
  appendChatLine(line);
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
  senderX = tsWidth + QtUi::style()->sepTsSender();
  textX = senderX + senderWidth + QtUi::style()->sepSenderText();
  tsGrabPos = tsWidth + (int)QtUi::style()->sepTsSender()/2;
  senderGrabPos = senderX + senderWidth + (int)QtUi::style()->sepSenderText()/2;
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
  if(y >= ycoords[ycoords.count()-1]) return ycoords.count()-2;
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
      default:
        break;
    }
  }
}

void ChatWidget::mouseDoubleClickEvent(QMouseEvent * /*event*/) {



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
  //MousePos oldpos = mousePos;
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
    default:
      break;
  }
  // Pass 2: Some mouse modes need work after being set...
  if(mouseMode == DragTsSep && x < size().width() - QtUi::style()->sepSenderText() - senderWidth - 10) {
    // Drag first column separator
    int foo = QtUi::style()->sepTsSender()/2;
    tsWidth = qMax(x, foo) - foo;
    computePositions();
    layout();
  } else if(mouseMode == DragTextSep && x < size().width() - 10) {
    // Drag second column separator
    int foo = tsWidth + QtUi::style()->sepTsSender() + QtUi::style()->sepSenderText()/2;
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
      result += QString("[%1] %2 %3\n").arg(lines[l]->timestamp().toLocalTime().toString("hh:mm:ss"))
  .        arg(lines[l]->sender()).arg(lines[l]->text());
    }
    return result;
  }
  // selectionMode == TextSelected
  return lines[selectionLine]->text().mid(selectionStart, selectionEnd - selectionStart);
}

