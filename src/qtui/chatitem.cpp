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

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QFontMetrics>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QTextLayout>

#include "chatitem.h"
#include "chatlinemodel.h"
#include "qtui.h"
#include "qtuistyle.h"

ChatItem::ChatItem(ChatLineModel::ColumnType col, QAbstractItemModel *model, QGraphicsItem *parent)
  : QGraphicsItem(parent),
    _fontMetrics(0),
    _selectionMode(NoSelection),
    _selectionStart(-1),
    _layout(0)
{
  Q_ASSERT(model);
  QModelIndex index = model->index(row(), col);
  _fontMetrics = QtUi::style()->fontMetrics(model->data(index, ChatLineModel::FormatRole).value<UiStyle::FormatList>().at(0).second);
  setAcceptHoverEvents(true);
  setZValue(20);
}

ChatItem::~ChatItem() {
  delete _layout;
}

QVariant ChatItem::data(int role) const {
  QModelIndex index = model()->index(row(), column());
  if(!index.isValid()) {
    qWarning() << "ChatItem::data(): model index is invalid!" << index;
    return QVariant();
  }
  return model()->data(index, role);
}

qreal ChatItem::setGeometry(qreal w, qreal h) {
  if(w == _boundingRect.width()) return _boundingRect.height();
  prepareGeometryChange();
  _boundingRect.setWidth(w);
  if(h < 0) h = computeHeight();
  _boundingRect.setHeight(h);
  if(haveLayout()) updateLayout();
  return h;
}

qreal ChatItem::computeHeight() {
  return fontMetrics()->lineSpacing(); // only contents can be multi-line
}

QTextLayout *ChatItem::createLayout(QTextOption::WrapMode wrapMode, Qt::Alignment alignment) {
  QTextLayout *layout = new QTextLayout(data(MessageModel::DisplayRole).toString());

  QTextOption option;
  option.setWrapMode(wrapMode);
  option.setAlignment(alignment);
  layout->setTextOption(option);

  QList<QTextLayout::FormatRange> formatRanges
         = QtUi::style()->toTextLayoutList(data(MessageModel::FormatRole).value<UiStyle::FormatList>(), layout->text().length());
  layout->setAdditionalFormats(formatRanges);
  return layout;
}

void ChatItem::updateLayout() {
  if(!haveLayout())
    setLayout(createLayout(QTextOption::WrapAnywhere, Qt::AlignLeft));

  layout()->beginLayout();
  QTextLine line = layout()->createLine();
  if(line.isValid()) {
    line.setLineWidth(width());
    line.setPosition(QPointF(0,0));
  }
  layout()->endLayout();
}

void ChatItem::clearLayout() {
  delete _layout;
  _layout = 0;
}

// NOTE: This is not the most time-efficient implementation, but it saves space by not caching unnecessary data
//       This is a deliberate trade-off. (-> selectFmt creation, data() call)
void ChatItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
  Q_UNUSED(option); Q_UNUSED(widget);
  if(!haveLayout()) updateLayout();
  painter->setClipRect(boundingRect()); // no idea why QGraphicsItem clipping won't work
  //if(_selectionMode == FullSelection) {
    //painter->save();
    //painter->fillRect(boundingRect(), QApplication::palette().brush(QPalette::Highlight));
    //painter->restore();
  //}
  QVector<QTextLayout::FormatRange> formats = additionalFormats();
  if(_selectionMode != NoSelection) {
    QTextLayout::FormatRange selectFmt;
    selectFmt.format.setForeground(QApplication::palette().brush(QPalette::HighlightedText));
    selectFmt.format.setBackground(QApplication::palette().brush(QPalette::Highlight));
    if(_selectionMode == PartialSelection) {
      selectFmt.start = qMin(_selectionStart, _selectionEnd);
      selectFmt.length = qAbs(_selectionStart - _selectionEnd);
    } else { // FullSelection
      selectFmt.start = 0;
      selectFmt.length = data(MessageModel::DisplayRole).toString().length();
    }
    formats.append(selectFmt);
  }
  layout()->draw(painter, QPointF(0,0), formats, boundingRect());
}

qint16 ChatItem::posToCursor(const QPointF &pos) {
  if(pos.y() > height()) return data(MessageModel::DisplayRole).toString().length();
  if(pos.y() < 0) return 0;
  if(!haveLayout()) updateLayout();
  for(int l = layout()->lineCount() - 1; l >= 0; l--) {
    QTextLine line = layout()->lineAt(l);
    if(pos.y() >= line.y()) {
      return line.xToCursor(pos.x(), QTextLine::CursorOnCharacter);
    }
  }
  return 0;
}

void ChatItem::setFullSelection() {
  if(_selectionMode != FullSelection) {
    _selectionMode = FullSelection;
    update();
  }
}

void ChatItem::clearSelection() {
  _selectionMode = NoSelection;
  update();
}

void ChatItem::continueSelecting(const QPointF &pos) {
  _selectionMode = PartialSelection;
  _selectionEnd = posToCursor(pos);
  update();
}

QList<QRectF> ChatItem::findWords(const QString &searchWord, Qt::CaseSensitivity caseSensitive) {
  QList<QRectF> resultList;
  const QAbstractItemModel *model_ = model();
  if(!model_)
    return resultList;

  QString plainText = model_->data(model_->index(row(), column()), MessageModel::DisplayRole).toString();
  QList<int> indexList;
  int searchIdx = plainText.indexOf(searchWord, 0, caseSensitive);
  while(searchIdx != -1) {
    indexList << searchIdx;
    searchIdx = plainText.indexOf(searchWord, searchIdx + 1, caseSensitive);
  }

  if(!haveLayout())
    updateLayout();

  foreach(int idx, indexList) {
    QTextLine line = layout()->lineForTextPosition(idx);
    qreal x = line.cursorToX(idx);
    qreal width = line.cursorToX(idx + searchWord.count()) - x;
    qreal height = fontMetrics()->lineSpacing();
    qreal y = height * line.lineNumber();
    resultList << QRectF(x, y, width, height);
  }
  return resultList;
}

void ChatItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
  if(event->buttons() == Qt::LeftButton) {
    chatScene()->setSelectingItem(this);
    _selectionStart = _selectionEnd = posToCursor(event->pos());
    _selectionMode = NoSelection; // will be set to PartialSelection by mouseMoveEvent
    update();
    event->accept();
  } else {
    event->ignore();
  }
}

void ChatItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
  if(event->buttons() == Qt::LeftButton) {
    if(contains(event->pos())) {
      qint16 end = posToCursor(event->pos());
      if(end != _selectionEnd) {
        _selectionEnd = end;
        _selectionMode = (_selectionStart != _selectionEnd ? PartialSelection : NoSelection);
        update();
      }
    } else {
      setFullSelection();
      chatScene()->startGlobalSelection(this, event->pos());
    }
    event->accept();
  } else {
    event->ignore();
  }
}

void ChatItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
  if(_selectionMode != NoSelection && !event->buttons() & Qt::LeftButton) {
    _selectionEnd = posToCursor(event->pos());
    QString selection
        = data(MessageModel::DisplayRole).toString().mid(qMin(_selectionStart, _selectionEnd), qAbs(_selectionStart - _selectionEnd));
    chatScene()->putToClipboard(selection);
    event->accept();
  } else {
    event->ignore();
  }
}

/*************************************************************************************************/

/*************************************************************************************************/

void SenderChatItem::updateLayout() {
  if(!haveLayout()) setLayout(createLayout(QTextOption::WrapAnywhere, Qt::AlignRight));
  ChatItem::updateLayout();
}

/*************************************************************************************************/

ContentsChatItem::ContentsChatItem(QAbstractItemModel *model, QGraphicsItem *parent) : ChatItem(column(), model, parent),
  _layoutData(0)
{

}

ContentsChatItem::~ContentsChatItem() {
  delete _layoutData;
}

qreal ContentsChatItem::computeHeight() {
  int lines = 1;
  WrapColumnFinder finder(this);
  while(finder.nextWrapColumn() > 0) lines++;
  return lines * fontMetrics()->lineSpacing();
}

void ContentsChatItem::setLayout(QTextLayout *layout) {
  if(!_layoutData) {
    _layoutData = new LayoutData;
    _layoutData->clickables = findClickables();
  } else {
    delete _layoutData->layout;
  }
  _layoutData->layout = layout;
}

void ContentsChatItem::clearLayout() {
  delete _layoutData;
  _layoutData = 0;
}

void ContentsChatItem::updateLayout() {
  if(!haveLayout()) setLayout(createLayout(QTextOption::WrapAnywhere));

  // Now layout
  ChatLineModel::WrapList wrapList = data(ChatLineModel::WrapListRole).value<ChatLineModel::WrapList>();
  if(!wrapList.count()) return; // empty chatitem

  qreal h = 0;
  WrapColumnFinder finder(this);
  layout()->beginLayout();
  forever {
    QTextLine line = layout()->createLine();
    if(!line.isValid())
      break;

    int col = finder.nextWrapColumn();
    line.setNumColumns(col >= 0 ? col - line.textStart() : layout()->text().length());
    line.setPosition(QPointF(0, h));
    h += fontMetrics()->lineSpacing();
  }
  layout()->endLayout();
}

// NOTE: This method is not threadsafe and not reentrant!
//       (RegExps are not constant while matching, and they are static here for efficiency)
QList<ContentsChatItem::Clickable> ContentsChatItem::findClickables() {
  // For matching URLs
  static QString urlEnd("(?:>|[,.;:\"]*\\s|\\b|$)");
  static QString urlChars("(?:[\\w\\-~@/?&=+$()!%#]|[,.;:]\\w)");

  static QRegExp regExp[] = {
    // URL
    // QRegExp(QString("((?:https?://|s?ftp://|irc://|mailto:|www\\.)%1+|%1+\\.[a-z]{2,4}(?:?=/%1+|\\b))%2").arg(urlChars, urlEnd)),
    QRegExp(QString("((?:(?:https?://|s?ftp://|irc://|mailto:)|www)%1+)%2").arg(urlChars, urlEnd)),

    // Channel name
    // We don't match for channel names starting with + or &, because that gives us a lot of false positives.
    QRegExp("((?:#|![A-Z0-9]{5})[^,:\\s]+(?::[^,:\\s]+)?)\\b")

    // TODO: Nicks, we'll need a filtering for only matching known nicknames further down if we do this
  };

  static const int regExpCount = 2;  // number of regexps in the array above

  qint16 matches[] = { 0, 0, 0 };
  qint16 matchEnd[] = { 0, 0, 0 };

  QString str = data(ChatLineModel::DisplayRole).toString();

  QList<Clickable> result;
  qint16 idx = 0;
  qint16 minidx;
  int type = -1;

  do {
    type = -1;
    minidx = str.length();
    for(int i = 0; i < regExpCount; i++) {
      if(matches[i] < 0 || matchEnd[i] > str.length()) continue;
      if(idx >= matchEnd[i]) {
        matches[i] = str.indexOf(regExp[i], qMax(matchEnd[i], idx));
        if(matches[i] >= 0) matchEnd[i] = matches[i] + regExp[i].cap(1).length();
      }
      if(matches[i] >= 0 && matches[i] < minidx) {
        minidx = matches[i];
        type = i;
      }
    }
    if(type >= 0) {
      idx = matchEnd[type];
      if(type == Clickable::Url && str.at(idx-1) == ')') {  // special case: closing paren only matches if we had an open one
        if(!str.mid(matches[type], matchEnd[type]-matches[type]).contains('(')) matchEnd[type]--;
      }
      result.append(Clickable((Clickable::Type)type, matches[type], matchEnd[type] - matches[type]));
    }
  } while(type >= 0);

  /* testing
  if(!result.isEmpty()) qDebug() << str;
  foreach(Clickable click, result) {
    qDebug() << str.mid(click.start, click.length);
  }
  */
  return result;
}

QVector<QTextLayout::FormatRange> ContentsChatItem::additionalFormats() const {
  // mark a clickable if hovered upon
  QVector<QTextLayout::FormatRange> fmt;
  if(layoutData()->currentClickable.isValid()) {
    Clickable click = layoutData()->currentClickable;
    QTextLayout::FormatRange f;
    f.start = click.start;
    f.length = click.length;
    f.format.setFontUnderline(true);
    fmt.append(f);
  }
  return fmt;
}

void ContentsChatItem::endHoverMode() {
  if(layoutData()->currentClickable.isValid()) {
    setCursor(Qt::ArrowCursor);
    layoutData()->currentClickable = Clickable();
    update();
  }
}

void ContentsChatItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
  layoutData()->hasDragged = false;
  ChatItem::mousePressEvent(event);
}

void ContentsChatItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
  if(!event->buttons() && !layoutData()->hasDragged) {
    // got a click
    Clickable click = layoutData()->currentClickable;
    if(click.isValid()) {
      QString str = data(ChatLineModel::DisplayRole).toString().mid(click.start, click.length);
      switch(click.type) {
        case Clickable::Url:
          QDesktopServices::openUrl(str);
          break;
        case Clickable::Channel:
          // TODO join or whatever...
          break;
        default:
          break;
      }
    }
  }
  ChatItem::mouseReleaseEvent(event);
}

void ContentsChatItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
  // mouse move events always mean we're not hovering anymore...
  endHoverMode();
  // also, check if we have dragged the mouse
  if(!layoutData()->hasDragged && event->buttons() & Qt::LeftButton
    && (event->buttonDownScreenPos(Qt::LeftButton) - event->screenPos()).manhattanLength() >= QApplication::startDragDistance())
    layoutData()->hasDragged = true;
  ChatItem::mouseMoveEvent(event);
}

void ContentsChatItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
  endHoverMode();
  event->accept();
}

void ContentsChatItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
  bool onClickable = false;
  qint16 idx = posToCursor(event->pos());
  for(int i = 0; i < layoutData()->clickables.count(); i++) {
    Clickable click = layoutData()->clickables.at(i);
    if(idx >= click.start && idx < click.start + click.length) {
      if(click.type == Clickable::Url)
        onClickable = true;
      else if(click.type == Clickable::Channel) {
        // TODO: don't make clickable if it's our own name
        //onClickable = true; //FIXME disabled for now
      }
      if(onClickable) {
        setCursor(Qt::PointingHandCursor);
        layoutData()->currentClickable = click;
        update();
        break;
      }
    }
  }
  if(!onClickable) endHoverMode();
  event->accept();
}

/*************************************************************************************************/

ContentsChatItem::WrapColumnFinder::WrapColumnFinder(ChatItem *_item)
  : item(_item),
    layout(0),
    wrapList(item->data(ChatLineModel::WrapListRole).value<ChatLineModel::WrapList>()),
    wordidx(0),
    lastwrapcol(0),
    lastwrappos(0),
    w(0)
{
}

ContentsChatItem::WrapColumnFinder::~WrapColumnFinder() {
  delete layout;
}

qint16 ContentsChatItem::WrapColumnFinder::nextWrapColumn() {
  while(wordidx < wrapList.count()) {
    w += wrapList.at(wordidx).width;
    if(w >= item->width()) {
      if(lastwrapcol >= wrapList.at(wordidx).start) {
        // first word, and it doesn't fit
        if(!line.isValid()) {
          layout = item->createLayout(QTextOption::NoWrap);
          layout->beginLayout();
          line = layout->createLine();
          line.setLineWidth(item->width());
          layout->endLayout();
        }
        int idx = line.xToCursor(lastwrappos + item->width(), QTextLine::CursorOnCharacter);
        qreal x = line.cursorToX(idx, QTextLine::Trailing);
        w = w - wrapList.at(wordidx).width - (x - lastwrappos);
        lastwrappos = x;
        lastwrapcol = idx;
        return idx;
      }
      // not the first word, so just wrap before this
      lastwrapcol = wrapList.at(wordidx).start;
      lastwrappos = lastwrappos + w - wrapList.at(wordidx).width;
      w = 0;
      return lastwrapcol;
    }
    w += wrapList.at(wordidx).trailing;
    wordidx++;
  }
  return -1;
}
