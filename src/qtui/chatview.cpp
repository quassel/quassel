/***************************************************************************
 *   Copyright (C) 2005-2022 by the Quassel Project                        *
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

#include "chatview.h"

#include <algorithm>

#include <QGraphicsTextItem>
#include <QKeyEvent>
#include <QMenu>
#include <QScrollBar>

#include "bufferwidget.h"
#include "chatline.h"
#include "chatscene.h"
#include "client.h"
#include "clientignorelistmanager.h"
#include "messagefilter.h"
#include "qtui.h"
#include "qtuistyle.h"
#include "util.h"

ChatView::ChatView(BufferId bufferId, QWidget* parent)
    : QGraphicsView(parent)
    , AbstractChatView()
{
    QList<BufferId> filterList;
    filterList.append(bufferId);
    auto* filter = new MessageFilter(Client::messageModel(), filterList, this);
    init(filter);
}

ChatView::ChatView(MessageFilter* filter, QWidget* parent)
    : QGraphicsView(parent)
    , AbstractChatView()
{
    init(filter);
}

void ChatView::init(MessageFilter* filter)
{
    _bufferContainer = nullptr;
    _currentScaleFactor = 1;
    _invalidateFilter = false;

    setAttribute(Qt::WA_AcceptTouchEvents);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setAlignment(Qt::AlignLeft | Qt::AlignBottom);
    setInteractive(true);
    // setOptimizationFlags(QGraphicsView::DontClipPainter | QGraphicsView::DontAdjustForAntialiasing);
    // setOptimizationFlags(QGraphicsView::DontAdjustForAntialiasing);
    setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    // setTransformationAnchor(QGraphicsView::NoAnchor);
    setTransformationAnchor(QGraphicsView::AnchorViewCenter);

    _scrollTimer.setInterval(100);
    _scrollTimer.setSingleShot(true);
    connect(&_scrollTimer, &QTimer::timeout, this, &ChatView::scrollTimerTimeout);

    _scene = new ChatScene(filter, filter->idString(), viewport()->width(), this);
    connect(_scene, &QGraphicsScene::sceneRectChanged, this, &ChatView::adjustSceneRect);
    connect(_scene, &ChatScene::lastLineChanged, this, &ChatView::lastLineChanged);
    connect(_scene, &ChatScene::mouseMoveWhileSelecting, this, &ChatView::mouseMoveWhileSelecting);
    setScene(_scene);

    connect(verticalScrollBar(), &QAbstractSlider::valueChanged, this, &ChatView::verticalScrollbarChanged);
    _lastScrollbarPos = verticalScrollBar()->maximum();

    // Workaround for the ChatView scrolling up a fair bit when scrollbar becomes visible
    verticalScrollBar()->installEventFilter(this);

    connect(Client::networkModel(), &NetworkModel::markerLineSet, this, &ChatView::markerLineSet);

    // only connect if client is synched with a core
    if (Client::isConnected())
        connect(Client::ignoreListManager(), &ClientIgnoreListManager::ignoreListChanged, this, &ChatView::invalidateFilter);
}

bool ChatView::event(QEvent* event)
{
    if (event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        switch (keyEvent->key()) {
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_PageUp:
        case Qt::Key_PageDown:
            if (requestBacklogForScroll()) {
                return true;
            }
        default:
            break;
        }
    }

    if (event->type() == QEvent::TouchBegin && ((QTouchEvent*)event)->device()->type() == QTouchDevice::TouchScreen) {
        // Enable scrolling by draging, disable selecting/clicking content
        setDragMode(QGraphicsView::ScrollHandDrag);
        setInteractive(false);
        // if scrollbar is not visible we need to request backlog below else we need to accept
        // the event now (return true) so that we will receive TouchUpdate and TouchEnd/TouchCancel
        if (verticalScrollBar()->isVisible())
            return true;
    }

    if (event->type() == QEvent::TouchEnd || event->type() == QEvent::TouchCancel) {
        // End scroll and reset settings to default
        setDragMode(QGraphicsView::NoDrag);
        setInteractive(true);
        _firstTouchUpdateHappened = false;
        return true;
    }

    if (event->type() == QEvent::TouchUpdate) {
        if (!_firstTouchUpdateHappened) {
            // After the first movement of a Touch-Point, calculate the distance in both axis
            // and if the point moved more horizontally abort scroll.
            QTouchEvent::TouchPoint p = ((QTouchEvent*)event)->touchPoints().at(0);
            double dx = qAbs(p.lastPos().x() - p.pos().x());
            double dy = qAbs(p.lastPos().y() - p.pos().y());
            if (dx > dy) {
                setDragMode(QGraphicsView::NoDrag);
                setInteractive(true);
            }
            _firstTouchUpdateHappened = true;
        }
        // Applying the movement happens automatically by the drag-mode
    }
    if (event->type() == QEvent::Wheel
        || (event->type() == QEvent::TouchBegin && ((QTouchEvent*)event)->device()->type() == QTouchDevice::TouchScreen)
        || event->type() == QEvent::TouchUpdate) {
        if (requestBacklogForScroll()) {
            return true;
        }
    }

    if (event->type() == QEvent::Show) {
        if (_invalidateFilter)
            invalidateFilter();
    }

    return QGraphicsView::event(event);
}

bool ChatView::eventFilter(QObject* watched, QEvent* event)
{
    QAbstractSlider* vbar = verticalScrollBar();
    Q_ASSERT(vbar);

    if (watched != vbar) {
        // Ignore and pass through all events not featuring the scrollbar
        return false;
    }
    if (event->type() == QEvent::Show) {
        // FIXME: Workaround for the ChatView scrolling up a fair bit when transitioning from the
        // vertical scrollbar not being visible, to becoming visible.  This happens especially
        // often when no initial backlog is loaded.
        if (_backlogRequestedBeforeScrollable) {
            _backlogRequestedBeforeScrollable = false;
            vbar->setValue(vbar->maximum());
        }
    }
    // Pass through all events
    return false;
}

void ChatView::resizeEvent(QResizeEvent* event)
{
    // if view is currently scrolled to bottom, we want it that way after resizing
    bool atBottom = (_lastScrollbarPos == verticalScrollBar()->maximum());

    QGraphicsView::resizeEvent(event);

    // if scrolling to bottom, do it immediately.
    if (atBottom) {
        // we can reduce viewport updates if we scroll to the bottom allready at the beginning
        verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    }

    scene()->updateForViewport(viewport()->width(), viewport()->height());
    adjustSceneRect();

    if (atBottom) {
        _lastScrollbarPos = verticalScrollBar()->maximum();
        verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    }
    checkChatLineCaches();
}

void ChatView::adjustSceneRect()
{
    // Workaround for QTBUG-6322
    // If the viewport's sceneRect() is (almost) as wide as as the viewport itself,
    // Qt wants to reserve space for scrollbars even if they're turned off, resulting in
    // an ugly white space at the bottom of the ChatView.
    // Since the view's scene's width actually doesn't matter at all, we just adjust it
    // by some hopefully large enough value to avoid this problem.

    setSceneRect(scene()->sceneRect().adjusted(0, 0, -25, 0));
}

void ChatView::mouseMoveWhileSelecting(const QPointF& scenePos)
{
    int y = (int)mapFromScene(scenePos).y();
    _scrollOffset = 0;
    if (y < 0)
        _scrollOffset = y;
    else if (y > height())
        _scrollOffset = y - height();

    if (_scrollOffset && !_scrollTimer.isActive())
        _scrollTimer.start();
}

void ChatView::scrollTimerTimeout()
{
    // scroll view
    QAbstractSlider* vbar = verticalScrollBar();
    if (_scrollOffset < 0 && vbar->value() > 0)
        vbar->setValue(qMax(vbar->value() + _scrollOffset, 0));
    else if (_scrollOffset > 0 && vbar->value() < vbar->maximum())
        vbar->setValue(qMin(vbar->value() + _scrollOffset, vbar->maximum()));
}

void ChatView::lastLineChanged(QGraphicsItem* chatLine, qreal offset)
{
    Q_UNUSED(chatLine)
    // disabled until further testing/discussion
    // if(!scene()->isScrollingAllowed())
    //  return;

    QAbstractSlider* vbar = verticalScrollBar();
    Q_ASSERT(vbar);
    if (vbar->maximum() - vbar->value() <= (offset + 5) * _currentScaleFactor) {  // 5px grace area
        vbar->setValue(vbar->maximum());
    }
}

void ChatView::verticalScrollbarChanged(int newPos)
{
    QAbstractSlider* vbar = verticalScrollBar();
    Q_ASSERT(vbar);

    // check for backlog request
    if (newPos < _lastScrollbarPos) {
        int relativePos = 100;
        if (vbar->maximum() - vbar->minimum() != 0)
            relativePos = (newPos - vbar->minimum()) * 100 / (vbar->maximum() - vbar->minimum());

        if (relativePos < 20) {
            scene()->requestBacklog();
        }
    }
    _lastScrollbarPos = newPos;

    // FIXME: Fugly workaround for the ChatView scrolling up 1px on buffer switch
    if (vbar->maximum() - newPos <= 2)
        vbar->setValue(vbar->maximum());
}

MsgId ChatView::lastMsgId() const
{
    if (!scene())
        return {};

    QAbstractItemModel* model = scene()->model();
    if (!model || model->rowCount() == 0)
        return {};

    return model->index(model->rowCount() - 1, 0).data(MessageModel::MsgIdRole).value<MsgId>();
}

MsgId ChatView::lastVisibleMsgId() const
{
    ChatLine* line = lastVisibleChatLine();

    if (line)
        return line->msgId();

    return {};
}

bool chatLinePtrLessThan(ChatLine* one, ChatLine* other)
{
    return one->row() < other->row();
}

// TODO: figure out if it's cheaper to use a cached list (that we'd need to keep updated)
QSet<ChatLine*> ChatView::visibleChatLines(Qt::ItemSelectionMode mode) const
{
    QSet<ChatLine*> result;
    foreach (QGraphicsItem* item, items(viewport()->rect().adjusted(-1, -1, 1, 1), mode)) {
        auto* line = qgraphicsitem_cast<ChatLine*>(item);
        if (line)
            result.insert(line);
    }
    return result;
}

QList<ChatLine*> ChatView::visibleChatLinesSorted(Qt::ItemSelectionMode mode) const
{
    QList<ChatLine*> result = visibleChatLines(mode).values();
    std::sort(result.begin(), result.end(), chatLinePtrLessThan);
    return result;
}

ChatLine* ChatView::lastVisibleChatLine(bool ignoreDayChange) const
{
    if (!scene())
        return nullptr;

    QAbstractItemModel* model = scene()->model();
    if (!model || model->rowCount() == 0)
        return nullptr;

    int row = -1;

    QSet<ChatLine*> visibleLines = visibleChatLines(Qt::ContainsItemBoundingRect);
    foreach (ChatLine* line, visibleLines) {
        if (line->row() > row && (ignoreDayChange ? line->msgType() != Message::DayChange : true))
            row = line->row();
    }

    if (row >= 0)
        return scene()->chatLine(row);

    return nullptr;
}

void ChatView::setMarkerLineVisible(bool visible)
{
    scene()->setMarkerLineVisible(visible);
}

void ChatView::setMarkerLine(MsgId msgId)
{
    if (!scene()->isSingleBufferScene())
        return;

    BufferId bufId = scene()->singleBufferId();
    Client::setMarkerLine(bufId, msgId);
}

void ChatView::markerLineSet(BufferId buffer, MsgId msgId)
{
    if (!scene()->isSingleBufferScene() || scene()->singleBufferId() != buffer)
        return;

    scene()->setMarkerLine(msgId);
    scene()->setMarkerLineVisible(true);
}

void ChatView::jumpToMarkerLine(bool requestBacklog)
{
    scene()->jumpToMarkerLine(requestBacklog);
}

void ChatView::addActionsToMenu(QMenu* menu, const QPointF& pos)
{
    // zoom actions
    auto* bw = qobject_cast<BufferWidget*>(bufferContainer());
    if (bw) {
        bw->addActionsToMenu(menu, pos);
        menu->addSeparator();
    }
}

void ChatView::zoomIn()
{
    _currentScaleFactor *= 1.2;
    scale(1.2, 1.2);
    scene()->setWidth(viewport()->width() / _currentScaleFactor - 2);
}

void ChatView::zoomOut()
{
    _currentScaleFactor /= 1.2;
    scale(1 / 1.2, 1 / 1.2);
    scene()->setWidth(viewport()->width() / _currentScaleFactor - 2);
}

void ChatView::zoomOriginal()
{
    scale(1 / _currentScaleFactor, 1 / _currentScaleFactor);
    _currentScaleFactor = 1;
    scene()->setWidth(viewport()->width() - 2);
}

void ChatView::invalidateFilter()
{
    // if this is the currently selected chatview
    // invalidate immediately
    if (isVisible()) {
        _scene->filter()->invalidateFilter();
        _invalidateFilter = false;
    }
    // otherwise invalidate whenever the view is shown
    else {
        _invalidateFilter = true;
    }
}

void ChatView::scrollContentsBy(int dx, int dy)
{
    QGraphicsView::scrollContentsBy(dx, dy);
    checkChatLineCaches();
}

void ChatView::setHasCache(ChatLine* line, bool hasCache)
{
    if (hasCache)
        _linesWithCache.insert(line);
    else
        _linesWithCache.remove(line);
}

bool ChatView::requestBacklogForScroll()
{
    if (!verticalScrollBar()->isVisible()) {
        // Not able to scroll, fetch backlog
        //
        // Future improvement: continue fetching backlog in chunks until the scrollbar is visible,
        // or the beginning of the buffer has been reached.
        scene()->requestBacklog();
        _backlogRequestedBeforeScrollable = true;
        // Backlog has been requested
        return true;
    }
    else {
        // Scrollbar already visible, no backlog requested
        return false;
    }
}

void ChatView::checkChatLineCaches()
{
    qreal top = mapToScene(viewport()->rect().topLeft()).y() - 10;  // some grace area to avoid premature cleaning
    qreal bottom = mapToScene(viewport()->rect().bottomRight()).y() + 10;
    QSet<ChatLine*>::iterator iter = _linesWithCache.begin();
    while (iter != _linesWithCache.end()) {
        ChatLine* line = *iter;
        if (line->pos().y() + line->height() < top || line->pos().y() > bottom) {
            line->clearCache();
            iter = _linesWithCache.erase(iter);
        }
        else
            ++iter;
    }
}
