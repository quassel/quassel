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

ChatWidget::ChatWidget(QWidget *parent) : QScrollArea(parent) {
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  setAlignment(Qt::AlignLeft | Qt::AlignTop);

}

void ChatWidget::init(QString netname, QString bufname, ChatWidgetContents *contentsWidget) {
  networkName = netname;
  bufferName = bufname;
  //setAlignment(Qt::AlignBottom);
  contents = contentsWidget;
  setWidget(contents);
  //setWidgetResizable(true);
  //contents->setWidth(contents->sizeHint().width());
  contents->setFocusProxy(this);
  //contents->show();
  //setAlignment(Qt::AlignBottom);
}

ChatWidget::~ChatWidget() {

}

void ChatWidget::clear() {
  //contents->clear();
}

void ChatWidget::appendMsg(Message msg) {
  contents->appendMsg(msg);
  //qDebug() << "appending" << msg.text;

}

void ChatWidget::resizeEvent(QResizeEvent *event) {
  //qDebug() << bufferName << isVisible() << event->size();
  contents->setWidth(event->size().width());
  //setAlignment(Qt::AlignBottom);
  QScrollArea::resizeEvent(event);
}

/*************************************************************************************/

ChatWidgetContents::ChatWidgetContents(QString net, QString buf, QWidget *parent) : QWidget(parent) {
  networkName = net;
  bufferName = buf;
  layoutTimer = new QTimer(this);
  layoutTimer->setSingleShot(true);
  connect(layoutTimer, SIGNAL(timeout()), this, SLOT(triggerLayout()));

  setBackgroundRole(QPalette::Base);
  setFont(QFont("Fixed"));

  ycoords.append(0);
  tsWidth = 90;
  senderWidth = 100;
  //textWidth = 400;
  computePositions();
  //setFixedWidth((int)(tsWidth + senderWidth + textWidth + 20));
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  setMouseTracking(true);
  mouseMode = Normal;
  selectionMode = NoSelection;

  doLayout = false;
}

ChatWidgetContents::~ChatWidgetContents() {
  delete layoutTimer;
  foreach(ChatLine *l, lines) {
    delete l;
  }
}

QSize ChatWidgetContents::sizeHint() const {
  //qDebug() << size();
  return size();
}

void ChatWidgetContents::paintEvent(QPaintEvent *event) {
  QPainter painter(this);
  qreal top = event->rect().top();
  qreal bot = top + event->rect().height();
  int idx = yToLineIdx(top);
  if(idx < 0) return;
  for(int i = idx; i < lines.count() ; i++) {
    lines[i]->draw(&painter, QPointF(0, ycoords[i]));
    if(ycoords[i+1] > bot) return;
  }
}

void ChatWidgetContents::appendMsg(Message msg) {
  ChatLine *line = new ChatLine(msg, networkName, bufferName);
  qreal h = line->layout(tsWidth, senderWidth, textWidth);
  ycoords.append(h + ycoords[ycoords.count() - 1]);
  setFixedHeight((int)ycoords[ycoords.count()-1]);
  lines.append(line);
  update();
  return;

}

void ChatWidgetContents::clear() {


}

//!\brief Computes the different x position vars for given tsWidth and senderWidth.
void ChatWidgetContents::computePositions() {
  senderX = tsWidth + Style::sepTsSender();
  textX = senderX + senderWidth + Style::sepSenderText();
  tsGrabPos = tsWidth + (int)Style::sepTsSender()/2;
  senderGrabPos = senderX + senderWidth + (int)Style::sepSenderText()/2;
  textWidth = size().width() - textX;
}

void ChatWidgetContents::setWidth(qreal w) {
  textWidth = (int)w - (Style::sepTsSender() + Style::sepSenderText()) - tsWidth - senderWidth;
  setFixedWidth((int)w);
  layout();
}

//!\brief Trigger layout (used by layoutTimer only).
/** This method is triggered by the layoutTimer. Layout the widget if it has been postponed earlier.
 */
void ChatWidgetContents::triggerLayout() {
  layout(true);
}

//!\brief Layout the widget.
/** The contents of the widget is re-layouted completely. Since this could take a while if the widget
 * is huge, we don't want to trigger the layout procedure too often (causing layout calls to pile up).
 * We use a timer that ensures layouting is only done if the last one has finished.
 */
void ChatWidgetContents::layout(bool timer) {
  if(layoutTimer->isActive()) {
    // Layouting too fast. We set a flag though, so that a final layout is done when the timer runs out.
    doLayout = true;
    return;
  }
  if(timer && !doLayout) return; // Only check doLayout if we have been triggered by the timer!
  qreal y = 0;
  for(int i = 0; i < lines.count(); i++) {
    qreal h = lines[i]->layout(tsWidth, senderWidth, textWidth);
    ycoords[i+1] = h + ycoords[i];
  }
  setFixedHeight((int)ycoords[ycoords.count()-1]);
  update();
  doLayout = false; // Clear previous layout requests
  layoutTimer->start(50); // Minimum time until we start the next layout
}

int ChatWidgetContents::yToLineIdx(qreal y) {
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

void ChatWidgetContents::mousePressEvent(QMouseEvent *event) {
  if(event->button() == Qt::LeftButton) {
    dragStartPos = event->pos();
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
          dragStartLine = yToLineIdx(event->pos().y());
          dragStartCursor = lines[dragStartLine]->posToCursor(QPointF(event->pos().x(), event->pos().y()-ycoords[dragStartLine]));
        }
        mouseMode = Pressed;
        break;
    }
  }
}

void ChatWidgetContents::mouseReleaseEvent(QMouseEvent *event) {
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
void ChatWidgetContents::mouseMoveEvent(QMouseEvent *event) {
  // Set some basic properties of the current position
  int x = event->pos().x();
  int y = event->pos().y();
  MousePos oldpos = mousePos;
  if(x >= tsGrabPos - 3 && x <= tsGrabPos + 3) mousePos = OverTsSep;
  else if(x >= senderGrabPos - 3 && x <= senderGrabPos + 3) mousePos = OverTextSep;
  else mousePos = None;

  // Pass 1: Do whatever we can before switching mouse mode (if at all).
  switch(mouseMode) {
    // No special mode. Set mouse cursor if appropriate.
    case Normal:
      if(oldpos != mousePos) {
        if(mousePos == OverTsSep || mousePos == OverTextSep) setCursor(Qt::OpenHandCursor);
        else setCursor(Qt::ArrowCursor);
      }
      break;
    // Left button pressed. Might initiate marking or drag & drop if we moved past the drag distance.
    case Pressed:
      if(!dragStartPos.isNull() && (dragStartPos - event->pos()).manhattanLength() >= QApplication::startDragDistance()) {
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
        update();
      }
    } else {
      mouseMode = MarkLines;
      selectionStart = qMin(curLine, dragStartLine); selectionEnd = qMax(curLine, dragStartLine);
      for(int i = selectionStart; i <= selectionEnd; i++) lines[i]->setSelection(ChatLine::Full);
      update();
    }
  } else if(mouseMode == MarkLines) {
    // Line marking
    int l = yToLineIdx(y);
    if(l != curLine) {
      selectionStart = qMin(l, dragStartLine); selectionEnd = qMax(l, dragStartLine);
      if(curLine >= 0 && curLine < selectionStart) {
        for(int i = curLine; i < selectionStart; i++) lines[i]->setSelection(ChatLine::None);
      } else if(curLine > selectionEnd) {
        for(int i = selectionEnd+1; i <= curLine; i++) lines[i]->setSelection(ChatLine::None);
      } else if(selectionStart < curLine && l < curLine) {
          for(int i = selectionStart; i < curLine; i++) lines[i]->setSelection(ChatLine::Full);
      } else if(curLine < selectionEnd && l > curLine) {
        for(int i = curLine+1; i <= selectionEnd; i++) lines[i]->setSelection(ChatLine::Full);
      }
      curLine = l;
      update();
    }
  }
}

//!\brief Clear current text selection.
void ChatWidgetContents::clearSelection() {
  if(selectionMode == TextSelected) {
    lines[selectionLine]->setSelection(ChatLine::None);
  } else if(selectionMode == LinesSelected) {
    for(int i = selectionStart; i <= selectionEnd; i++) {
      lines[i]->setSelection(ChatLine::None);
    }
  }
  selectionMode = NoSelection;
  update();
}

//!\brief Convert current selection to human-readable string.
QString ChatWidgetContents::selectionToString() {
  //TODO Make selection format configurable!
  if(selectionMode == NoSelection) return "";
  if(selectionMode == LinesSelected) {
    QString result;
    for(int l = selectionStart; l <= selectionEnd; l++) {
      result += QString("[%1] %2 %3\n").arg(lines[l]->getTimeStamp().toLocalTime().toString("hh:mm:ss"))
          .arg(lines[l]->getSender()).arg(lines[l]->getText());
    }
    return result;
  }
  // selectionMode == TextSelected
  return lines[selectionLine]->getText().mid(selectionStart, selectionEnd - selectionStart);
}

/************************************************************************************/

//!\brief Construct a ChatLine object from a message.
/**
 * \param m The message to be layouted and rendered
 * \param net The network name
 * \param buf The buffer name
 */ 
ChatLine::ChatLine(Message m, QString net, QString buf) : QObject() {
  hght = 0;
  networkName = net;
  bufferName = buf;
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
      s = tr("%Dj-->"); t = tr("%Dj%DN%1%DN %DH(%2@%3)%DH has joined %DC%4%DC").arg(nick, user, host, bufferName); break;
    case Message::Part:
      s = tr("%Dp<--"); t = tr("%Dp%DN%1%DN %DH(%2@%3)%DH has left %DC%4%DC").arg(nick, user, host, bufferName);
      if(!text.isEmpty()) t = QString("%1 (%2)").arg(t).arg(text);
      break;
    case Message::Quit:
      s = tr("%Dq<--"); t = tr("%Dq%DN%1%DN %DH(%2@%3)%DH has quit").arg(nick, user, host);
      if(!text.isEmpty()) t = QString("%1 (%2)").arg(t).arg(text);
      break;
    case Message::Kick:
      { s = tr("%Dk<-*");
        QString victim = text.section(" ", 0, 0);
        //if(victim == ui.ownNick->currentText()) victim = tr("you");
        QString kickmsg = text.section(" ", 1);
        t = tr("%Dk%DN%1%DN has kicked %DN%2%DN from %DC%3%DC").arg(nick).arg(victim).arg(bufferName);
        if(!kickmsg.isEmpty()) t = QString("%1 (%2)").arg(t).arg(kickmsg);
      }
      break;
    case Message::Nick:
      s = tr("%Dr<->");
      if(nick == msg.text) t = tr("You are now known as %DN%1%DN").arg(msg.text);
      else t = tr("%DN%1%DN is now known as %DN%2%DN").arg(nick, msg.text);
      break;
    case Message::Mode:
      s = tr("%Dm***");
      if(nick.isEmpty()) t = tr("User mode: %DM%1%DM").arg(msg.text);
      else t = tr("Mode %DM%1%DM by %DN%2%DN").arg(msg.text, nick);
      break;
    default:
      s = tr("%De%1").arg(msg.sender);
      t = tr("%De[%1]").arg(msg.text);
  }
  QTextOption tsOption, senderOption, textOption;
  tsFormatted = Style::internalToFormatted(c);
  senderFormatted = Style::internalToFormatted(s);
  textFormatted = Style::internalToFormatted(t);
  tsLayout.setText(tsFormatted.text); tsLayout.setAdditionalFormats(tsFormatted.formats);
  tsOption.setWrapMode(QTextOption::NoWrap);
  tsLayout.setTextOption(tsOption);
  senderLayout.setText(senderFormatted.text); senderLayout.setAdditionalFormats(senderFormatted.formats);
  senderOption.setAlignment(Qt::AlignRight); senderOption.setWrapMode(QTextOption::ManualWrap);
  senderLayout.setTextOption(senderOption);
  textLayout.setText(textFormatted.text); textLayout.setAdditionalFormats(textFormatted.formats);
  textOption.setWrapMode(QTextOption::WrapAnywhere); // seems to do what we want, apparently
  textLayout.setTextOption(textOption);
}

//!\brief Return the cursor position for the given coordinate pos.
/**
 * \param pos The position relative to the ChatLine
 * \return The cursor position, or -2 for timestamp, or -1 for sender
 */
int ChatLine::posToCursor(QPointF pos) {
  if(pos.x() < tsWidth + (int)Style::sepTsSender()/2) return -2;
  qreal textStart = tsWidth + Style::sepTsSender() + senderWidth + Style::sepSenderText();
  if(pos.x() < textStart) return -1;
  for(int l = textLayout.lineCount() - 1; l >=0; l--) {
    QTextLine line = textLayout.lineAt(l);
    if(pos.y() >= line.position().y()) {
      int p = line.xToCursor(pos.x() - textStart, QTextLine::CursorOnCharacter);
      return p;
    }
  }
}

void ChatLine::setSelection(SelectionMode mode, int start, int end) {
  selectionMode = mode;
  tsFormat.clear(); senderFormat.clear(); textFormat.clear();
  QPalette pal = QApplication::palette();
  QTextLayout::FormatRange tsSel, senderSel, textSel;
  switch (mode) {
    case None:
      break;
    case Partial:
      selectionStart = qMin(start, end); selectionEnd = qMax(start, end);
      textSel.format.setForeground(pal.brush(QPalette::HighlightedText));
      textSel.format.setBackground(pal.brush(QPalette::Highlight));
      textSel.start = selectionStart;
      textSel.length = selectionEnd - selectionStart;
      textFormat.append(textSel);
      break;
    case Full:
      tsSel.format.setForeground(pal.brush(QPalette::HighlightedText));
      tsSel.start = 0; tsSel.length = tsLayout.text().length(); tsFormat.append(tsSel);
      senderSel.format.setForeground(pal.brush(QPalette::HighlightedText));
      senderSel.start = 0; senderSel.length = senderLayout.text().length(); senderFormat.append(senderSel);
      textSel.format.setForeground(pal.brush(QPalette::HighlightedText));
      textSel.start = 0; textSel.length = textLayout.text().length(); textFormat.append(textSel);
      break;
  }
}

QDateTime ChatLine::getTimeStamp() {
  return msg.timeStamp;
}

QString ChatLine::getSender() {
  return senderLayout.text();
}

QString ChatLine::getText() {
  return textLayout.text();
}

qreal ChatLine::layout(qreal tsw, qreal senderw, qreal textw) {
  tsWidth = tsw; senderWidth = senderw; textWidth = textw;
  QTextLine tl;
  tsLayout.beginLayout();
  tl = tsLayout.createLine();
  tl.setLineWidth(tsWidth);
  tl.setPosition(QPointF(0, 0));
  tsLayout.endLayout();

  senderLayout.beginLayout();
  tl = senderLayout.createLine();
  tl.setLineWidth(senderWidth);
  tl.setPosition(QPointF(0, 0));
  senderLayout.endLayout();

  qreal h = 0;
  textLayout.beginLayout();
  while(1) {
    tl = textLayout.createLine();
    if(!tl.isValid()) break;
    tl.setLineWidth(textWidth);
    tl.setPosition(QPointF(0, h));
    h += tl.height();
  }
  textLayout.endLayout();
  hght = h;
  return h;
}

//!\brief Draw ChatLine on the given QPainter at the given position.
void ChatLine::draw(QPainter *p, const QPointF &pos) {
  QPalette pal = QApplication::palette();
  if(selectionMode == Full) {
    p->setPen(Qt::NoPen);
    p->setBrush(pal.brush(QPalette::Highlight));
    p->drawRect(QRectF(pos, QSizeF(tsWidth + Style::sepTsSender() + senderWidth + Style::sepSenderText() + textWidth, height())));
  } else if(selectionMode == Partial) {

  }
  p->setClipRect(QRectF(pos, QSizeF(tsWidth, height())));
  tsLayout.draw(p, pos, tsFormat);
  p->setClipRect(QRectF(pos + QPointF(tsWidth + Style::sepTsSender(), 0), QSizeF(senderWidth, height())));
  senderLayout.draw(p, pos + QPointF(tsWidth + Style::sepTsSender(), 0), senderFormat);
  p->setClipping(false);
  textLayout.draw(p, pos + QPointF(tsWidth + Style::sepTsSender() + senderWidth + Style::sepSenderText(), 0), textFormat);
}
