/***************************************************************************
 *   Copyright (C) 2005-2016 by the Quassel Project                        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDrag>
#include <QGraphicsSceneMouseEvent>
#include <QIcon>
#include <QMenu>
#include <QMenuBar>
#include <QMimeData>
#include <QPersistentModelIndex>
#include <QUrl>

#ifdef HAVE_KDE4
#  include <KMenuBar>
#else
#  include <QMenuBar>
#endif

#ifdef HAVE_WEBENGINE
#  include <QWebEngineView>
#elif defined HAVE_WEBKIT
#  include <QWebView>
#endif

#include "chatitem.h"
#include "chatline.h"
#include "chatlinemodelitem.h"
#include "chatscene.h"
#include "chatview.h"
#include "client.h"
#include "clientbacklogmanager.h"
#include "columnhandleitem.h"
#include "contextmenuactionprovider.h"
#include "mainwin.h"
#include "markerlineitem.h"
#include "messagefilter.h"
#include "qtui.h"
#include "qtuistyle.h"
#include "chatviewsettings.h"
#include "webpreviewitem.h"

const qreal minContentsWidth = 200;

ChatScene::ChatScene(QAbstractItemModel *model, const QString &idString, qreal width, ChatView *parent)
    : QGraphicsScene(0, 0, width, 0, (QObject *)parent),
    _chatView(parent),
    _idString(idString),
    _model(model),
    _singleBufferId(BufferId()),
    _sceneRect(0, 0, width, 0),
    _firstLineRow(-1),
    _viewportHeight(0),
    _markerLine(new MarkerLineItem(width)),
    _markerLineVisible(false),
    _markerLineValid(false),
    _markerLineJumpPending(false),
    _cutoffMode(CutoffRight),
    _selectingItem(0),
    _selectionStart(-1),
    _isSelecting(false),
    _clickMode(NoClick),
    _clickHandled(true),
    _leftButtonPressed(false)
{
    MessageFilter *filter = qobject_cast<MessageFilter *>(model);
    if (filter && filter->isSingleBufferFilter()) {
        _singleBufferId = filter->singleBufferId();
    }

    addItem(_markerLine);
    connect(this, SIGNAL(sceneRectChanged(const QRectF &)), _markerLine, SLOT(sceneRectChanged(const QRectF &)));

    ChatViewSettings defaultSettings;
    _defaultFirstColHandlePos = defaultSettings.value("FirstColumnHandlePos", 80).toInt();
    _defaultSecondColHandlePos = defaultSettings.value("SecondColumnHandlePos", 200).toInt();

    ChatViewSettings viewSettings(this);
    _firstColHandlePos = viewSettings.value("FirstColumnHandlePos", _defaultFirstColHandlePos).toInt();
    _secondColHandlePos = viewSettings.value("SecondColumnHandlePos", _defaultSecondColHandlePos).toInt();

    _firstColHandle = new ColumnHandleItem(QtUi::style()->firstColumnSeparator());
    addItem(_firstColHandle);
    _firstColHandle->setXPos(_firstColHandlePos);
    connect(_firstColHandle, SIGNAL(positionChanged(qreal)), this, SLOT(firstHandlePositionChanged(qreal)));
    connect(this, SIGNAL(sceneRectChanged(const QRectF &)), _firstColHandle, SLOT(sceneRectChanged(const QRectF &)));

    _secondColHandle = new ColumnHandleItem(QtUi::style()->secondColumnSeparator());
    addItem(_secondColHandle);
    _secondColHandle->setXPos(_secondColHandlePos);
    connect(_secondColHandle, SIGNAL(positionChanged(qreal)), this, SLOT(secondHandlePositionChanged(qreal)));

    connect(this, SIGNAL(sceneRectChanged(const QRectF &)), _secondColHandle, SLOT(sceneRectChanged(const QRectF &)));

    setHandleXLimits();

    if (model->rowCount() > 0)
        rowsInserted(QModelIndex(), 0, model->rowCount() - 1);

    connect(model, SIGNAL(rowsInserted(const QModelIndex &, int, int)),
        this, SLOT(rowsInserted(const QModelIndex &, int, int)));
    connect(model, SIGNAL(rowsAboutToBeRemoved(const QModelIndex &, int, int)),
        this, SLOT(rowsAboutToBeRemoved(const QModelIndex &, int, int)));
    connect(model, SIGNAL(rowsRemoved(QModelIndex, int, int)),
        this, SLOT(rowsRemoved()));
    connect(model, SIGNAL(dataChanged(QModelIndex, QModelIndex)), SLOT(dataChanged(QModelIndex, QModelIndex)));

#if defined HAVE_WEBKIT || defined HAVE_WEBENGINE
    webPreview.timer.setSingleShot(true);
    connect(&webPreview.timer, SIGNAL(timeout()), this, SLOT(webPreviewNextStep()));
#endif
    _showWebPreview = defaultSettings.showWebPreview();
    defaultSettings.notify("ShowWebPreview", this, SLOT(showWebPreviewChanged()));

    _showSenderBrackets = defaultSettings.showSenderBrackets();
    defaultSettings.notify("ShowSenderBrackets", this, SLOT(showSenderBracketsChanged()));

    _timestampFormatString = defaultSettings.timestampFormatString();
    defaultSettings.notify("TimestampFormat", this, SLOT(timestampFormatStringChanged()));
    updateTimestampHasBrackets();

    _clickTimer.setInterval(QApplication::doubleClickInterval());
    _clickTimer.setSingleShot(true);
    connect(&_clickTimer, SIGNAL(timeout()), SLOT(clickTimeout()));

    setItemIndexMethod(QGraphicsScene::NoIndex);
}


ChatScene::~ChatScene()
{
}


ChatView *ChatScene::chatView() const
{
    return _chatView;
}


ColumnHandleItem *ChatScene::firstColumnHandle() const
{
    return _firstColHandle;
}


ColumnHandleItem *ChatScene::secondColumnHandle() const
{
    return _secondColHandle;
}

void ChatScene::resetColumnWidths()
{
    //make sure first column is at least 80 px wide, second 120 px
    int firstColHandlePos = qMax(_defaultFirstColHandlePos,
                                 80);
    int secondColHandlePos = qMax(_defaultSecondColHandlePos,
                                  firstColHandlePos + 120);

    _firstColHandle->setXPos(firstColHandlePos);
    _secondColHandle->setXPos(secondColHandlePos);
}

ChatLine *ChatScene::chatLine(MsgId msgId, bool matchExact, bool ignoreDayChange) const
{
    if (!_lines.count())
        return 0;

    QList<ChatLine *>::ConstIterator start = _lines.begin();
    QList<ChatLine *>::ConstIterator end = _lines.end();
    QList<ChatLine *>::ConstIterator middle;

    int n = int(end - start);
    int half;

    while (n > 0) {
        half = n >> 1;
        middle = start + half;
        if ((*middle)->msgId() < msgId) {
            start = middle + 1;
            n -= half + 1;
        }
        else {
            n = half;
        }
    }

    if (start != end && (*start)->msgId() == msgId && (ignoreDayChange ? (*start)->msgType() != Message::DayChange : true))
        return *start;

    if (matchExact)
        return 0;

    if (start == _lines.begin()) // not (yet?) in our scene
        return 0;

    // if we didn't find the exact msgId, take the next-lower one (this makes sense for lastSeen)

    if (start == end) { // higher than last element
        if (!ignoreDayChange)
            return _lines.last();

        for (int i = _lines.count() -1; i >= 0; i--) {
            if (_lines.at(i)->msgType() != Message::DayChange)
                return _lines.at(i);
        }
        return 0;
    }

    // return the next-lower line
    if (!ignoreDayChange)
        return *(--start);

    do {
        if ((*(--start))->msgType() != Message::DayChange)
            return *start;
    }
    while (start != _lines.begin());
    return 0;
}


ChatItem *ChatScene::chatItemAt(const QPointF &scenePos) const
{
    foreach(QGraphicsItem *item, items(scenePos, Qt::IntersectsItemBoundingRect, Qt::AscendingOrder)) {
        ChatLine *line = qgraphicsitem_cast<ChatLine *>(item);
        if (line)
            return line->itemAt(line->mapFromScene(scenePos));
    }
    return 0;
}


bool ChatScene::containsBuffer(const BufferId &id) const
{
    MessageFilter *filter = qobject_cast<MessageFilter *>(model());
    if (filter)
        return filter->containsBuffer(id);
    else
        return false;
}


void ChatScene::setMarkerLineVisible(bool visible)
{
    _markerLineVisible = visible;
    if (visible && _markerLineValid)
        markerLine()->setVisible(true);
    else
        markerLine()->setVisible(false);
}


void ChatScene::setMarkerLine(MsgId msgId)
{
    if (!isSingleBufferScene())
        return;

    if (!msgId.isValid())
        msgId = Client::markerLine(singleBufferId());

    if (msgId.isValid()) {
        ChatLine *line = chatLine(msgId, false, true);
        if (line) {
            markerLine()->setChatLine(line);
            // if this was the last line, we won't see it because it's outside the sceneRect
            // .. which is exactly what we want :)
            markerLine()->setPos(line->pos() + QPointF(0, line->height()));

            // DayChange messages might have been hidden outside the scene rect, don't make the markerline visible then!
            if (markerLine()->pos().y() >= sceneRect().y()) {
                _markerLineValid = true;
                if (_markerLineVisible)
                    markerLine()->setVisible(true);
                if (_markerLineJumpPending) {
                    _markerLineJumpPending = false;
                    if (markerLine()->isVisible()) {
                        markerLine()->ensureVisible(QRectF(), 50, 50);
                    }
                }
                return;
            }
        }
    }
    _markerLineValid = false;
    markerLine()->setVisible(false);
}


void ChatScene::jumpToMarkerLine(bool requestBacklog)
{
    if (!isSingleBufferScene())
        return;

    if (markerLine()->isVisible()) {
        markerLine()->ensureVisible(QRectF(), 50, 50);
        return;
    }
    if (!_markerLineValid && requestBacklog) {
        MsgId msgId = Client::markerLine(singleBufferId());
        if (msgId.isValid()) {
            _markerLineJumpPending = true;
            Client::backlogManager()->requestBacklog(singleBufferId(), msgId, -1, -1, 0);

            // If we filtered out the lastSeenMsg (by changing filters after setting it), we'd never jump because the above request
            // won't fetch any prior lines. Thus, trigger a dynamic backlog request just in case, so repeated
            // jump tries will eventually cause enough backlog to be fetched.
            // This is a bit hackish, but not wasteful, as jumping to the top of the ChatView would trigger a dynamic fetch anyway.
            this->requestBacklog();
        }
    }
}


void ChatScene::rowsInserted(const QModelIndex &index, int start, int end)
{
    Q_UNUSED(index);

//   QModelIndex sidx = model()->index(start, 2);
//   QModelIndex eidx = model()->index(end, 2);
//   qDebug() << "rowsInserted:";
//   if(start > 0) {
//     QModelIndex ssidx = model()->index(start - 1, 2);
//     qDebug() << "Start--:" << start - 1 << ssidx.data(MessageModel::MsgIdRole).value<MsgId>()
//           << ssidx.data(Qt::DisplayRole).toString();
//   }
//   qDebug() << "Start:" << start << sidx.data(MessageModel::MsgIdRole).value<MsgId>()
//         << sidx.data(Qt::DisplayRole).toString();
//   qDebug() << "End:" << end << eidx.data(MessageModel::MsgIdRole).value<MsgId>()
//         << eidx.data(Qt::DisplayRole).toString();
//   if(end + 1 < model()->rowCount()) {
//     QModelIndex eeidx = model()->index(end + 1, 2);
//     qDebug() << "End++:" << end + 1 << eeidx.data(MessageModel::MsgIdRole).value<MsgId>()
//           << eeidx.data(Qt::DisplayRole).toString();
//   }

    qreal h = 0;
    qreal y = 0;
    qreal width = _sceneRect.width();
    bool atBottom = (start == _lines.count());
    bool atTop = !atBottom && (start == 0);

    if (start < _lines.count()) {
        y = _lines.value(start)->y();
    }
    else if (atBottom && !_lines.isEmpty()) {
        y = _lines.last()->y() + _lines.last()->height();
    }

    qreal contentsWidth = width - secondColumnHandle()->sceneRight();
    qreal senderWidth = secondColumnHandle()->sceneLeft() - firstColumnHandle()->sceneRight();
    qreal timestampWidth = firstColumnHandle()->sceneLeft();
    QPointF contentsPos(secondColumnHandle()->sceneRight(), 0);
    QPointF senderPos(firstColumnHandle()->sceneRight(), 0);

    if (atTop) {
        for (int i = end; i >= start; i--) {
            ChatLine *line = new ChatLine(i, model(),
                width,
                timestampWidth, senderWidth, contentsWidth,
                senderPos, contentsPos);
            h += line->height();
            line->setPos(0, y-h);
            _lines.insert(start, line);
            addItem(line);
        }
    }
    else {
        for (int i = start; i <= end; i++) {
            ChatLine *line = new ChatLine(i, model(),
                width,
                timestampWidth, senderWidth, contentsWidth,
                senderPos, contentsPos);
            line->setPos(0, y+h);
            h += line->height();
            _lines.insert(i, line);
            addItem(line);
        }
    }

    // update existing items
    for (int i = end+1; i < _lines.count(); i++) {
        _lines[i]->setRow(i);
    }

    // update selection
    if (_selectionStart >= 0) {
        int offset = end - start + 1;
        int oldStart = _selectionStart;
        if (_selectionStart >= start)
            _selectionStart += offset;
        if (_selectionEnd >= start) {
            _selectionEnd += offset;
            if (_selectionStart == oldStart)
                for (int i = start; i < start + offset; i++)
                    _lines[i]->setSelected(true);
        }
        if (_firstSelectionRow >= start)
            _firstSelectionRow += offset;
    }

    // neither pre- or append means we have to do dirty work: move items...
    if (!(atTop || atBottom)) {
        ChatLine *line = 0;
        for (int i = 0; i <= end; i++) {
            line = _lines.at(i);
            line->setPos(0, line->pos().y() - h);
            if (line == markerLine()->chatLine())
                markerLine()->setPos(line->pos() + QPointF(0, line->height()));
        }
    }

    // check if all went right
    Q_ASSERT(start == 0 || _lines.at(start - 1)->pos().y() + _lines.at(start - 1)->height() == _lines.at(start)->pos().y());
//   if(start != 0) {
//     if(_lines.at(start - 1)->pos().y() + _lines.at(start - 1)->height() != _lines.at(start)->pos().y()) {
//       qDebug() << "lines:" << _lines.count() << "start:" << start << "end:" << end;
//       qDebug() << "line[start - 1]:" << _lines.at(start - 1)->pos().y() << "+" << _lines.at(start - 1)->height() << "=" << _lines.at(start - 1)->pos().y() + _lines.at(start - 1)->height();
//       qDebug() << "line[start]" << _lines.at(start)->pos().y();
//       qDebug() << "needed moving:" << !(atTop || atBottom) << moveTop << moveStart << moveEnd << offset;
//       Q_ASSERT(false)
//     }
//   }
    Q_ASSERT(end + 1 == _lines.count() || _lines.at(end)->pos().y() + _lines.at(end)->height() == _lines.at(end + 1)->pos().y());
//   if(end + 1 < _lines.count()) {
//     if(_lines.at(end)->pos().y() + _lines.at(end)->height() != _lines.at(end + 1)->pos().y()) {
//       qDebug() << "lines:" << _lines.count() << "start:" << start << "end:" << end;
//       qDebug() << "line[end]:" << _lines.at(end)->pos().y() << "+" << _lines.at(end)->height() << "=" << _lines.at(end)->pos().y() + _lines.at(end)->height();
//       qDebug() << "line[end+1]" << _lines.at(end + 1)->pos().y();
//       qDebug() << "needed moving:" << !(atTop || atBottom) << moveTop << moveStart << moveEnd << offset;
//       Q_ASSERT(false);
//     }
//   }

    if (!atBottom) {
        if (start < _firstLineRow) {
            int prevFirstLineRow = _firstLineRow + (end - start + 1);
            for (int i = end + 1; i < prevFirstLineRow; i++) {
                _lines.at(i)->show();
            }
        }
        // force new search for first proper line
        _firstLineRow = -1;
    }
    updateSceneRect();
    if (atBottom) {
        emit lastLineChanged(_lines.last(), h);
    }

    // now move the marker line if necessary. we don't need to do anything if we appended lines though...
    if (!_markerLineValid)
        setMarkerLine();
}


void ChatScene::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    Q_UNUSED(parent);

    qreal h = 0; // total height of removed items;

    bool atTop = (start == 0);
    bool atBottom = (end == _lines.count() - 1);

    // clear selection
    if (_selectingItem) {
        int row = _selectingItem->row();
        if (row >= start && row <= end)
            setSelectingItem(0);
    }

    // remove items from scene
    QList<ChatLine *>::iterator lineIter = _lines.begin() + start;
    int lineCount = start;
    while (lineIter != _lines.end() && lineCount <= end) {
        if ((*lineIter) == markerLine()->chatLine())
            markerLine()->setChatLine(0);
        h += (*lineIter)->height();
        delete *lineIter;
        lineIter = _lines.erase(lineIter);
        lineCount++;
    }

    // update rows of remaining chatlines
    for (int i = start; i < _lines.count(); i++) {
        _lines.at(i)->setRow(i);
    }

    // update selection
    if (_selectionStart >= 0) {
        int offset = end - start + 1;
        if (_selectionStart >= start)
            _selectionStart = qMax(_selectionStart - offset, start);
        if (_selectionEnd >= start)
            _selectionEnd -= offset;
        if (_firstSelectionRow >= start)
            _firstSelectionRow -= offset;

        if (_selectionEnd < _selectionStart) {
            _isSelecting = false;
            _selectionStart = -1;
        }
    }

    // neither removing at bottom or top means we have to move items...
    if (!(atTop || atBottom)) {
        qreal offset = h;
        int moveStart = 0;
        int moveEnd = _lines.count() - 1;
        if (start < _lines.count() - start) {
            // move top part
            moveEnd = start - 1;
        }
        else {
            // move bottom part
            moveStart = start;
            offset = -offset;
        }
        ChatLine *line = 0;
        for (int i = moveStart; i <= moveEnd; i++) {
            line = _lines.at(i);
            line->setPos(0, line->pos().y() + offset);
        }
    }

    Q_ASSERT(start == 0 || start >= _lines.count() || _lines.at(start - 1)->pos().y() + _lines.at(start - 1)->height() == _lines.at(start)->pos().y());

    // update sceneRect
    // when searching for the first non-date-line we have to take into account that our
    // model still contains the just removed lines so we cannot simply call updateSceneRect()
    int numRows = model()->rowCount();
    QModelIndex firstLineIdx;
    _firstLineRow = -1;
    bool needOffset = false;
    do {
        _firstLineRow++;
        if (_firstLineRow >= start && _firstLineRow <= end) {
            _firstLineRow = end + 1;
            needOffset = true;
        }
        firstLineIdx = model()->index(_firstLineRow, 0);
    }
    while ((Message::Type)(model()->data(firstLineIdx, MessageModel::TypeRole).toInt()) == Message::DayChange && _firstLineRow < numRows);

    if (needOffset)
        _firstLineRow -= end - start + 1;
    updateSceneRect();
}


void ChatScene::rowsRemoved()
{
    // move the marker line if necessary
    setMarkerLine();
}


void ChatScene::dataChanged(const QModelIndex &tl, const QModelIndex &br)
{
    layout(tl.row(), br.row(), _sceneRect.width());
}


void ChatScene::updateForViewport(qreal width, qreal height)
{
    _viewportHeight = height;
    setWidth(width);
}


void ChatScene::setWidth(qreal width)
{
    if (width == _sceneRect.width())
        return;
    layout(0, _lines.count()-1, width);
}


void ChatScene::layout(int start, int end, qreal width)
{
    // clock_t startT = clock();

    // disabling the index while doing this complex updates is about
    // 2 to 10 times faster!
    //setItemIndexMethod(QGraphicsScene::NoIndex);

    if (end >= 0) {
        int row = end;
        qreal linePos = _lines.at(row)->scenePos().y() + _lines.at(row)->height();
        qreal contentsWidth = width - secondColumnHandle()->sceneRight();
        while (row >= start) {
            _lines.at(row--)->setGeometryByWidth(width, contentsWidth, linePos);
        }

        if (row >= 0) {
            // remaining items don't need geometry changes, but maybe repositioning?
            ChatLine *line = _lines.at(row);
            qreal offset = linePos - (line->scenePos().y() + line->height());
            if (offset != 0) {
                while (row >= 0) {
                    line = _lines.at(row--);
                    line->setPos(0, line->scenePos().y() + offset);
                }
            }
        }
    }

    //setItemIndexMethod(QGraphicsScene::BspTreeIndex);

    updateSceneRect(width);
    setHandleXLimits();
    setMarkerLine();
    emit layoutChanged();

//   clock_t endT = clock();
//   qDebug() << "resized" << _lines.count() << "in" << (float)(endT - startT) / CLOCKS_PER_SEC << "sec";
}


void ChatScene::firstHandlePositionChanged(qreal xpos)
{
    if (_firstColHandlePos == xpos)
        return;

    _firstColHandlePos = xpos >= 0 ? xpos : 0;
    ChatViewSettings viewSettings(this);
    viewSettings.setValue("FirstColumnHandlePos", _firstColHandlePos);
    ChatViewSettings defaultSettings;
    defaultSettings.setValue("FirstColumnHandlePos", _firstColHandlePos);

    // clock_t startT = clock();

    // disabling the index while doing this complex updates is about
    // 2 to 10 times faster!
    //setItemIndexMethod(QGraphicsScene::NoIndex);

    QList<ChatLine *>::iterator lineIter = _lines.end();
    QList<ChatLine *>::iterator lineIterBegin = _lines.begin();
    qreal timestampWidth = firstColumnHandle()->sceneLeft();
    qreal senderWidth = secondColumnHandle()->sceneLeft() - firstColumnHandle()->sceneRight();
    QPointF senderPos(firstColumnHandle()->sceneRight(), 0);

    while (lineIter != lineIterBegin) {
        --lineIter;
        (*lineIter)->setFirstColumn(timestampWidth, senderWidth, senderPos);
    }
    //setItemIndexMethod(QGraphicsScene::BspTreeIndex);

    setHandleXLimits();

//   clock_t endT = clock();
//   qDebug() << "resized" << _lines.count() << "in" << (float)(endT - startT) / CLOCKS_PER_SEC << "sec";
}


void ChatScene::secondHandlePositionChanged(qreal xpos)
{
    if (_secondColHandlePos == xpos)
        return;

    _secondColHandlePos = xpos;
    ChatViewSettings viewSettings(this);
    viewSettings.setValue("SecondColumnHandlePos", _secondColHandlePos);
    ChatViewSettings defaultSettings;
    defaultSettings.setValue("SecondColumnHandlePos", _secondColHandlePos);

    // clock_t startT = clock();

    // disabling the index while doing this complex updates is about
    // 2 to 10 times faster!
    //setItemIndexMethod(QGraphicsScene::NoIndex);

    QList<ChatLine *>::iterator lineIter = _lines.end();
    QList<ChatLine *>::iterator lineIterBegin = _lines.begin();
    qreal linePos = _sceneRect.y() + _sceneRect.height();
    qreal senderWidth = secondColumnHandle()->sceneLeft() - firstColumnHandle()->sceneRight();
    qreal contentsWidth = _sceneRect.width() - secondColumnHandle()->sceneRight();
    QPointF contentsPos(secondColumnHandle()->sceneRight(), 0);
    while (lineIter != lineIterBegin) {
        --lineIter;
        (*lineIter)->setSecondColumn(senderWidth, contentsWidth, contentsPos, linePos);
    }
    //setItemIndexMethod(QGraphicsScene::BspTreeIndex);

    updateSceneRect();
    setHandleXLimits();
    emit layoutChanged();

//   clock_t endT = clock();
//   qDebug() << "resized" << _lines.count() << "in" << (float)(endT - startT) / CLOCKS_PER_SEC << "sec";
}


void ChatScene::setHandleXLimits()
{
    _firstColHandle->setXLimits(0, _secondColHandle->sceneLeft());
    _secondColHandle->setXLimits(_firstColHandle->sceneRight(), width() - minContentsWidth);
    update();
}


void ChatScene::setSelectingItem(ChatItem *item)
{
    if (_selectingItem) _selectingItem->clearSelection();
    _selectingItem = item;
}


void ChatScene::startGlobalSelection(ChatItem *item, const QPointF &itemPos)
{
    _selectionStart = _selectionEnd = _firstSelectionRow = item->row();
    _selectionStartCol = _selectionMinCol = item->column();
    _isSelecting = true;
    _lines[_selectionStart]->setSelected(true, (ChatLineModel::ColumnType)_selectionMinCol);
    updateSelection(item->mapToScene(itemPos));
}


void ChatScene::updateSelection(const QPointF &pos)
{
    int curRow = rowByScenePos(pos);
    if (curRow < 0) return;
    int curColumn = (int)columnByScenePos(pos);
    ChatLineModel::ColumnType minColumn = (ChatLineModel::ColumnType)qMin(curColumn, _selectionStartCol);
    if (minColumn != _selectionMinCol) {
        _selectionMinCol = minColumn;
        for (int l = qMin(_selectionStart, _selectionEnd); l <= qMax(_selectionStart, _selectionEnd); l++) {
            _lines[l]->setSelected(true, minColumn);
        }
    }
    int newstart = qMin(curRow, _firstSelectionRow);
    int newend = qMax(curRow, _firstSelectionRow);
    if (newstart < _selectionStart) {
        for (int l = newstart; l < _selectionStart; l++)
            _lines[l]->setSelected(true, minColumn);
    }
    if (newstart > _selectionStart) {
        for (int l = _selectionStart; l < newstart; l++)
            _lines[l]->setSelected(false);
    }
    if (newend > _selectionEnd) {
        for (int l = _selectionEnd+1; l <= newend; l++)
            _lines[l]->setSelected(true, minColumn);
    }
    if (newend < _selectionEnd) {
        for (int l = newend+1; l <= _selectionEnd; l++)
            _lines[l]->setSelected(false);
    }

    _selectionStart = newstart;
    _selectionEnd = newend;

    if (newstart == newend && minColumn == ChatLineModel::ContentsColumn) {
        if (!_selectingItem) {
            // _selectingItem has been removed already
            return;
        }
        _lines[curRow]->setSelected(false);
        _isSelecting = false;
        _selectionStart = -1;
        _selectingItem->continueSelecting(_selectingItem->mapFromScene(pos));
    }
}


bool ChatScene::isPosOverSelection(const QPointF &pos) const
{
    ChatItem *chatItem = chatItemAt(pos);
    if (!chatItem)
        return false;
    if (hasGlobalSelection()) {
        int row = chatItem->row();
        if (row >= qMin(_selectionStart, _selectionEnd) && row <= qMax(_selectionStart, _selectionEnd))
            return columnByScenePos(pos) >= _selectionMinCol;
    }
    else {
        return chatItem->isPosOverSelection(chatItem->mapFromScene(pos));
    }
    return false;
}


bool ChatScene::isScrollingAllowed() const
{
    if (_isSelecting)
        return false;

    // TODO: Handle clicks and single-item selections too

    return true;
}


/******** MOUSE HANDLING **************************************************************************/

void ChatScene::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    QPointF pos = event->scenePos();
    QMenu menu;

    // zoom actions and similar
    chatView()->addActionsToMenu(&menu, pos);
    menu.addSeparator();

    // item-specific options (select link etc)
    ChatItem *item = chatItemAt(pos);
    if (item)
        item->addActionsToMenu(&menu, item->mapFromScene(pos));
    else
        // no item -> default scene actions
        GraphicalUi::contextMenuActionProvider()->addActions(&menu, filter(), BufferId());

    // If we have text selected, insert the Copy Selection as first item
    if (isPosOverSelection(pos)) {
        QAction *sep = menu.insertSeparator(menu.actions().first());
        QAction *act = new Action(QIcon::fromTheme("edit-copy"), tr("Copy Selection"), &menu, this,
            SLOT(selectionToClipboard()), QKeySequence::Copy);
        menu.insertAction(sep, act);

        QString searchSelectionText = selection();
        if (searchSelectionText.length() > _webSearchSelectionTextMaxVisible)
            searchSelectionText = searchSelectionText.left(_webSearchSelectionTextMaxVisible).append(QString::fromUtf8("…"));
        searchSelectionText = tr("Search '%1'").arg(searchSelectionText);

        QAction *webSearchAction = new Action(QIcon::fromTheme("edit-find"), searchSelectionText, &menu, this, SLOT(webSearchOnSelection()));
        menu.insertAction(sep, webSearchAction);
    }

    if (QtUi::mainWindow()->menuBar()->isHidden())
        menu.addAction(QtUi::actionCollection("General")->action("ToggleMenuBar"));

    // show column reset action if columns have been resized in this session or there is at least one very narrow column
    if ((_firstColHandlePos != _defaultFirstColHandlePos) || (_secondColHandlePos != _defaultSecondColHandlePos) ||
        (_firstColHandlePos <= 10) || (_secondColHandlePos - _firstColHandlePos <= 10))
        menu.addAction(new Action(tr("Reset Column Widths"), &menu, this, SLOT(resetColumnWidths()), 0));

    menu.exec(event->screenPos());
}


void ChatScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton) {
        if (!_clickHandled && (event->scenePos() - _clickPos).toPoint().manhattanLength() >= QApplication::startDragDistance()) {
            if (_clickTimer.isActive())
                _clickTimer.stop();
            if (_clickMode == SingleClick && isPosOverSelection(_clickPos))
                initiateDrag(event->widget());
            else {
                _clickMode = DragStartClick;
                handleClick(Qt::LeftButton, _clickPos);
            }
            _clickMode = NoClick;
        }
        if (_isSelecting) {
            updateSelection(event->scenePos());
            emit mouseMoveWhileSelecting(event->scenePos());
            event->accept();
        }
        else if (_clickHandled && _clickMode < DoubleClick)
            QGraphicsScene::mouseMoveEvent(event);
    }
    else
        QGraphicsScene::mouseMoveEvent(event);
}


void ChatScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton) {
        _leftButtonPressed = true;
        _clickHandled = false;
        if (!isPosOverSelection(event->scenePos())) {
            // immediately clear selection if clicked outside; otherwise, wait for potential drag
            clearSelection();
        }
        if (_clickMode != NoClick && _clickTimer.isActive()) {
            switch (_clickMode) {
            case NoClick:
                _clickMode = SingleClick; break;
            case SingleClick:
                _clickMode = DoubleClick; break;
            case DoubleClick:
                _clickMode = TripleClick; break;
            case TripleClick:
                _clickMode = DoubleClick; break;
            case DragStartClick:
                break;
            }
            handleClick(Qt::LeftButton, _clickPos);
        }
        else {
            _clickMode = SingleClick;
            _clickPos = event->scenePos();
        }
        _clickTimer.start();
    }
    if (event->type() == QEvent::GraphicsSceneMouseDoubleClick)
        QGraphicsScene::mouseDoubleClickEvent(event);
    else
        QGraphicsScene::mousePressEvent(event);
}


void ChatScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    // we check for doubleclick ourselves, so just call press handler
    mousePressEvent(event);
}


void ChatScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && _leftButtonPressed) {
        _leftButtonPressed = false;
        if (_clickMode != NoClick) {
            if (_clickMode == SingleClick)
                clearSelection();
            event->accept();
            if (!_clickTimer.isActive())
                handleClick(Qt::LeftButton, _clickPos);
        }
        else {
            // no click -> drag or selection move
            if (isGloballySelecting()) {
                selectionToClipboard(QClipboard::Selection);
                _isSelecting = false;
                event->accept();
                return;
            }
        }
    }
    QGraphicsScene::mouseReleaseEvent(event);
}


void ChatScene::clickTimeout()
{
    if (!_leftButtonPressed && _clickMode == SingleClick)
        handleClick(Qt::LeftButton, _clickPos);
}


void ChatScene::handleClick(Qt::MouseButton button, const QPointF &scenePos)
{
    if (button == Qt::LeftButton) {
        clearSelection();

        // Now send click down to items
        ChatItem *chatItem = chatItemAt(scenePos);
        if (chatItem) {
            chatItem->handleClick(chatItem->mapFromScene(scenePos), _clickMode);
        }
        _clickHandled = true;
    }
}


void ChatScene::initiateDrag(QWidget *source)
{
    QDrag *drag = new QDrag(source);
    QMimeData *mimeData = new QMimeData;
    mimeData->setText(selection());
    drag->setMimeData(mimeData);

    drag->exec(Qt::CopyAction);
}


/******** SELECTIONS ******************************************************************************/

void ChatScene::selectionToClipboard(QClipboard::Mode mode)
{
    if (!hasSelection())
        return;

    stringToClipboard(selection(), mode);
}


void ChatScene::stringToClipboard(const QString &str_, QClipboard::Mode mode)
{
    QString str = str_;
    // remove trailing linefeeds
    if (str.endsWith('\n'))
        str.chop(1);

    switch (mode) {
    case QClipboard::Clipboard:
        QApplication::clipboard()->setText(str);
        break;
    case QClipboard::Selection:
        if (QApplication::clipboard()->supportsSelection())
            QApplication::clipboard()->setText(str, QClipboard::Selection);
        break;
    default:
        break;
    };
}


//!\brief Convert current selection to human-readable string.
QString ChatScene::selection() const
{
    //TODO Make selection format configurable!
    if (hasGlobalSelection()) {
        int start = qMin(_selectionStart, _selectionEnd);
        int end = qMax(_selectionStart, _selectionEnd);
        if (start < 0 || end >= _lines.count()) {
            qDebug() << "Invalid selection range:" << start << end;
            return QString();
        }
        QString result;

        for (int l = start; l <= end; l++) {
            if (_selectionMinCol == ChatLineModel::TimestampColumn) {
                ChatItem *item = _lines[l]->item(ChatLineModel::TimestampColumn);
                if (!_showSenderBrackets && !_timestampHasBrackets) {
                    // Only re-add brackets if the current timestamp format does not include them
                    // -and- sender brackets are disabled.  Don't filter on Message::Plain as
                    // timestamp brackets affect all types of messages.
                    // Remove any spaces before and after, otherwise it may look weird.
                    result += QString("[%1] ").arg(item->data(MessageModel::DisplayRole)
                                                   .toString().trimmed());
                } else {
                    result += item->data(MessageModel::DisplayRole).toString() + " ";
                }
            }
            if (_selectionMinCol <= ChatLineModel::SenderColumn) {
                ChatItem *item = _lines[l]->item(ChatLineModel::SenderColumn);
                if (!_showSenderBrackets && item->chatLine()->msgType() == Message::Plain) {
                    // Copying to plain-text.  Only re-add the sender brackets if they're normally
                    // hidden.
                    result += QString("<%1> ").arg(item->data(MessageModel::DisplayRole)
                                                   .toString());
                } else {
                    result += item->data(MessageModel::DisplayRole).toString() + " ";
                }
            }
            result += _lines[l]->item(ChatLineModel::ContentsColumn)
                    ->data(MessageModel::DisplayRole).toString() + "\n";
        }
        return result;
    }
    else if (selectingItem())
        return selectingItem()->selection();
    return QString();
}


bool ChatScene::hasSelection() const
{
    return hasGlobalSelection() || (selectingItem() && selectingItem()->hasSelection());
}


bool ChatScene::hasGlobalSelection() const
{
    return _selectionStart >= 0;
}


bool ChatScene::isGloballySelecting() const
{
    return _isSelecting;
}


void ChatScene::clearGlobalSelection()
{
    if (hasGlobalSelection()) {
        for (int l = qMin(_selectionStart, _selectionEnd); l <= qMax(_selectionStart, _selectionEnd); l++)
            _lines[l]->setSelected(false);
        _isSelecting = false;
        _selectionStart = -1;
    }
}


void ChatScene::clearSelection()
{
    clearGlobalSelection();
    if (selectingItem())
        selectingItem()->clearSelection();
}


/******** *************************************************************************************/

void ChatScene::webSearchOnSelection()
{
    if (!hasSelection())
        return;

    ChatViewSettings settings;
    QString webSearchBaseUrl = settings.webSearchUrlFormatString();
    QString webSearchUrl = webSearchBaseUrl.replace(QString("%s"), selection());
    QUrl url = QUrl::fromUserInput(webSearchUrl);
    QDesktopServices::openUrl(url);
}


/******** *************************************************************************************/

void ChatScene::requestBacklog()
{
    MessageFilter *filter = qobject_cast<MessageFilter *>(model());
    if (filter)
        return filter->requestBacklog();
    return;
}


ChatLineModel::ColumnType ChatScene::columnByScenePos(qreal x) const
{
    if (x < _firstColHandle->x())
        return ChatLineModel::TimestampColumn;
    if (x < _secondColHandle->x())
        return ChatLineModel::SenderColumn;

    return ChatLineModel::ContentsColumn;
}


int ChatScene::rowByScenePos(qreal y) const
{
    QList<QGraphicsItem *> itemList = items(QPointF(0, y));

    // ChatLine should be at the bottom of the list
    for (int i = itemList.count()-1; i >= 0; i--) {
        ChatLine *line = qgraphicsitem_cast<ChatLine *>(itemList.at(i));
        if (line)
            return line->row();
    }
    return -1;
}


void ChatScene::updateSceneRect(qreal width)
{
    if (_lines.isEmpty()) {
        updateSceneRect(QRectF(0, 0, width, 0));
        return;
    }

    // we hide day change messages at the top by making the scene rect smaller
    // and by calling QGraphicsItem::hide() on all leading day change messages
    // the first one is needed to ensure proper scrollbar ranges
    // the second for cases where the viewport is larger then the set scenerect
    //  (in this case the items are shown anyways)
    if (_firstLineRow == -1) {
        int numRows = model()->rowCount();
        _firstLineRow = 0;
        QModelIndex firstLineIdx;
        while (_firstLineRow < numRows) {
            firstLineIdx = model()->index(_firstLineRow, 0);
            if ((Message::Type)(model()->data(firstLineIdx, MessageModel::TypeRole).toInt()) != Message::DayChange)
                break;
            _lines.at(_firstLineRow)->hide();
            _firstLineRow++;
        }
    }

    // the following call should be safe. If it crashes something went wrong during insert/remove
    if (_firstLineRow < _lines.count()) {
        ChatLine *firstLine = _lines.at(_firstLineRow);
        ChatLine *lastLine = _lines.last();
        updateSceneRect(QRectF(0, firstLine->pos().y(), width, lastLine->pos().y() + lastLine->height() - firstLine->pos().y()));
    }
    else {
        // empty scene rect
        updateSceneRect(QRectF(0, 0, width, 0));
    }
}


void ChatScene::updateSceneRect(const QRectF &rect)
{
    _sceneRect = rect;
    setSceneRect(rect);
    update();
}


// ========================================
//  Webkit/WebEngine Only stuff
// ========================================
#if defined HAVE_WEBKIT || defined HAVE_WEBENGINE
void ChatScene::loadWebPreview(ChatItem *parentItem, const QUrl &url, const QRectF &urlRect)
{
    if (!_showWebPreview)
        return;

    if (webPreview.urlRect != urlRect)
        webPreview.urlRect = urlRect;

    if (webPreview.parentItem != parentItem)
        webPreview.parentItem = parentItem;

    if (webPreview.url != url) {
        webPreview.url = url;
        // prepare to load a different URL
        if (webPreview.previewItem) {
            if (webPreview.previewItem->scene())
                removeItem(webPreview.previewItem);
            delete webPreview.previewItem;
            webPreview.previewItem = 0;
        }
        webPreview.previewState = WebPreview::NoPreview;
    }

    if (webPreview.url.isEmpty())
        return;

    // qDebug() << Q_FUNC_INFO << webPreview.previewState;
    switch (webPreview.previewState) {
    case WebPreview::NoPreview:
        webPreview.previewState = WebPreview::NewPreview;
        webPreview.timer.start(500);
        break;
    case WebPreview::NewPreview:
    case WebPreview::DelayPreview:
    case WebPreview::ShowPreview:
        // we're already waiting for the next step or showing the preview
        break;
    case WebPreview::HidePreview:
        // we still have a valid preview
        webPreview.previewState = WebPreview::DelayPreview;
        webPreview.timer.start(1000);
        break;
    }
    // qDebug() << "  new State:" << webPreview.previewState << webPreview.timer.isActive();
}


void ChatScene::webPreviewNextStep()
{
    // qDebug() << Q_FUNC_INFO << webPreview.previewState;
    switch (webPreview.previewState) {
    case WebPreview::NoPreview:
        break;
    case WebPreview::NewPreview:
        Q_ASSERT(!webPreview.previewItem);
        webPreview.previewItem = new WebPreviewItem(webPreview.url);
        webPreview.previewState = WebPreview::DelayPreview;
        webPreview.timer.start(1000);
        break;
    case WebPreview::DelayPreview:
        Q_ASSERT(webPreview.previewItem);
        // calc position and show
        {
            qreal previewY = webPreview.urlRect.bottom();
            qreal previewX = webPreview.urlRect.x();
            if (previewY + webPreview.previewItem->boundingRect().height() > sceneRect().bottom())
                previewY = webPreview.urlRect.y() - webPreview.previewItem->boundingRect().height();

            if (previewX + webPreview.previewItem->boundingRect().width() > sceneRect().width())
                previewX = sceneRect().right() - webPreview.previewItem->boundingRect().width();

            webPreview.previewItem->setPos(previewX, previewY);
        }
        addItem(webPreview.previewItem);
        webPreview.previewState = WebPreview::ShowPreview;
        break;
    case WebPreview::ShowPreview:
        qWarning() << "ChatScene::webPreviewNextStep() called while in ShowPreview Step!";
        qWarning() << "removing preview";
        if (webPreview.previewItem && webPreview.previewItem->scene())
            removeItem(webPreview.previewItem);
    // Fall through to deletion!
    case WebPreview::HidePreview:
        if (webPreview.previewItem) {
            delete webPreview.previewItem;
            webPreview.previewItem = 0;
        }
        webPreview.parentItem = 0;
        webPreview.url = QUrl();
        webPreview.urlRect = QRectF();
        webPreview.previewState = WebPreview::NoPreview;
    }
    // qDebug() << "  new State:" << webPreview.previewState << webPreview.timer.isActive();
}


void ChatScene::clearWebPreview(ChatItem *parentItem)
{
    // qDebug() << Q_FUNC_INFO << webPreview.previewState;
    switch (webPreview.previewState) {
    case WebPreview::NewPreview:
        webPreview.previewState = WebPreview::NoPreview; // we haven't loaded anything yet
        break;
    case WebPreview::ShowPreview:
        if (parentItem == 0 || webPreview.parentItem == parentItem) {
            if (webPreview.previewItem && webPreview.previewItem->scene())
                removeItem(webPreview.previewItem);
        }
    // fall through into to set hidden state
    case WebPreview::DelayPreview:
        // we're just loading, so haven't shown the preview yet.
        webPreview.previewState = WebPreview::HidePreview;
        webPreview.timer.start(5000);
        break;
    case WebPreview::NoPreview:
    case WebPreview::HidePreview:
        break;
    }
    // qDebug() << "  new State:" << webPreview.previewState << webPreview.timer.isActive();
}


#endif

// ========================================
//  end of webkit only
// ========================================

// Local configuration caching
void ChatScene::showWebPreviewChanged()
{
    ChatViewSettings settings;
    _showWebPreview = settings.showWebPreview();
}

void ChatScene::showSenderBracketsChanged()
{
    ChatViewSettings settings;
    _showSenderBrackets = settings.showSenderBrackets();
}

void ChatScene::timestampFormatStringChanged()
{
    ChatViewSettings settings;
    _timestampFormatString = settings.timestampFormatString();
    updateTimestampHasBrackets();
}

void ChatScene::updateTimestampHasBrackets()
{
    // Calculate these parameters only as needed, rather than on-demand

    // Does the timestamp format contain brackets?  For example:
    // Classic: "[hh:mm:ss]"
    // Modern:  " hh:mm:ss"
    //
    // Match groups of any opening or closing brackets - (), {}, [], <>, (>, {], etc:
    //   ^\s*[({[<].+[)}\]>]\s*$
    //   [...]    is a character group containing ...
    //   ^        matches start of string
    //   \s*      matches any amount of whitespace
    //   [({[<]   matches (, {, [, or <
    //   .+       matches one or more characters
    //   [)}\]>]  matches ), }, ], or >, escaping the ]
    //   $        matches end of string
    // Alternatively, if opening and closing brackets must be in pairs, use this:
    //   (^\s*\(.+\)\s*$)|(^\s*\{.+\}\s*$)|(^\s*\[.+\]\s*$)|(^\s*<.+>\s*$)
    // Note that '\' must be escaped as '\\'
    // Helpful interactive website for debugging and explaining:  https://regex101.com/
    const QRegExp regExpMatchBrackets("^\\s*[({[<].+[)}\\]>]\\s*$");
    _timestampHasBrackets = regExpMatchBrackets.exactMatch(_timestampFormatString);
}
