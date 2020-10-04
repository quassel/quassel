/***************************************************************************
 *   Copyright (C) 2005-2020 by the Quassel Project                        *
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

#ifndef CHATVIEW_H_
#define CHATVIEW_H_

#include <QGraphicsView>
#include <QTimer>

#include "abstractbuffercontainer.h"

class AbstractBufferContainer;
class AbstractUiMsg;
class Buffer;
class ChatLine;
class ChatScene;
class MessageFilter;
class QMenu;

class ChatView : public QGraphicsView, public AbstractChatView
{
    Q_OBJECT

public:
    ChatView(MessageFilter*, QWidget* parent = nullptr);
    ChatView(BufferId bufferId, QWidget* parent = nullptr);

    MsgId lastMsgId() const override;
    virtual MsgId lastVisibleMsgId() const;
    inline AbstractBufferContainer* bufferContainer() const { return _bufferContainer; }
    inline void setBufferContainer(AbstractBufferContainer* c) { _bufferContainer = c; }

    inline ChatScene* scene() const { return _scene; }

    //! Return a set of ChatLines currently visible in the view
    /** \param mode How partially visible ChatLines are handled
     *  \return A set of visible ChatLines
     */
    QSet<ChatLine*> visibleChatLines(Qt::ItemSelectionMode mode = Qt::ContainsItemBoundingRect) const;

    //! Return a sorted list of ChatLines currently visible in the view
    /** \param mode How partially visible ChatLines are handled
     *  \return A list of visible ChatLines sorted by row
     *  \note If the order of ChatLines does not matter, use visibleChatLines() instead
     */
    QList<ChatLine*> visibleChatLinesSorted(Qt::ItemSelectionMode mode = Qt::ContainsItemBoundingRect) const;

    //! Return the last fully visible ChatLine in this view
    /** Using this method more efficient than calling visibleChatLinesSorted() and taking its last element.
     *  \return The last fully visible ChatLine in the view
     */
    ChatLine* lastVisibleChatLine(bool ignoreDayChange = false) const;

    virtual void addActionsToMenu(QMenu*, const QPointF& pos);

    //! Tell the view that this ChatLine has cached data
    /** ChatLines cache some layout data that should be cleared as soon as it's no
     *  longer visible. A ChatLine caching data registers itself with this method to
     *  tell the view about it. The view will call ChatLine::clearCache() when
     *  appropriate.
     *  \param line The ChatLine having cached data
     */
    void setHasCache(ChatLine* line, bool hasCache = true);

    /**
     * Requests backlog if the scrollbar is not currently visible
     *
     * Use this whenever trying to scroll the backlog to try to ensure some text is visible.  If the
     * backlog does not have additional messages or those messages are filtered out, the scrollbar
     * might remain invisible.
     *
     * @return True if the scrollbar isn't visible and a backlog request was made, otherwise false
     */
    bool requestBacklogForScroll();

public slots:
    inline virtual void clear() {}
    void zoomIn();
    void zoomOut();
    void zoomOriginal();

    void setMarkerLineVisible(bool visible = true);
    void setMarkerLine(MsgId msgId);
    void jumpToMarkerLine(bool requestBacklog);

protected:
    bool event(QEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void scrollContentsBy(int dx, int dy) override;

protected slots:
    virtual void verticalScrollbarChanged(int);

private slots:
    void lastLineChanged(QGraphicsItem* chatLine, qreal offset);
    void adjustSceneRect();
    void checkChatLineCaches();
    void mouseMoveWhileSelecting(const QPointF& scenePos);
    void scrollTimerTimeout();
    void invalidateFilter();
    void markerLineSet(BufferId buffer, MsgId msg);

private:
    void init(MessageFilter* filter);

    AbstractBufferContainer* _bufferContainer;
    ChatScene* _scene;
    int _lastScrollbarPos;
    qreal _currentScaleFactor;
    QTimer _scrollTimer;
    int _scrollOffset;
    bool _invalidateFilter;
    QSet<ChatLine*> _linesWithCache;
    bool _firstTouchUpdateHappened = false;
    /// Workaround: If true, backlog has been requested before the vertical scrollbar became visible
    bool _backlogRequestedBeforeScrollable{false};
};

#endif
