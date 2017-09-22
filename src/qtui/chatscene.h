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

#ifndef CHATSCENE_H_
#define CHATSCENE_H_

#include <QAbstractItemModel>
#include <QClipboard>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QSet>
#include <QTimer>
#include <QUrl>

#include "chatlinemodel.h"
#include "messagefilter.h"

class AbstractUiMsg;
class ChatItem;
class ChatLine;
class ChatView;
class ColumnHandleItem;
class MarkerLineItem;
class WebPreviewItem;

class QGraphicsSceneMouseEvent;

class ChatScene : public QGraphicsScene
{
    Q_OBJECT

public:
    enum CutoffMode {
        CutoffLeft,
        CutoffRight
    };

    enum ItemType {
        ChatLineType = QGraphicsItem::UserType + 1,
        ChatItemType,
        TimestampChatItemType,
        SenderChatItemType,
        ContentsChatItemType,
        SearchHighlightType,
        WebPreviewType,
        ColumnHandleType,
        MarkerLineType
    };

    enum ClickMode {
        NoClick,
        DragStartClick,
        SingleClick,
        DoubleClick,
        TripleClick
    };

    ChatScene(QAbstractItemModel *model, const QString &idString, qreal width, ChatView *parent);
    virtual ~ChatScene();

    inline QAbstractItemModel *model() const { return _model; }
    inline MessageFilter *filter() const { return qobject_cast<MessageFilter *>(_model); }
    inline QString idString() const { return _idString; }

    int rowByScenePos(qreal y) const;
    inline int rowByScenePos(const QPointF &pos) const { return rowByScenePos(pos.y()); }
    ChatLineModel::ColumnType columnByScenePos(qreal x) const;
    inline ChatLineModel::ColumnType columnByScenePos(const QPointF &pos) const { return columnByScenePos(pos.x()); }

    ChatView *chatView() const;
    ChatItem *chatItemAt(const QPointF &pos) const;
    inline ChatLine *chatLine(int row) const { return (row < _lines.count()) ? _lines.value(row) : 0; }
    inline ChatLine *chatLine(const QModelIndex &index) const { return _lines.value(index.row()); }

    //! Find the ChatLine belonging to a MsgId
    /** Searches for the ChatLine belonging to a MsgId. If there are more than one ChatLine with the same msgId,
     *  the first one is returned.
     *  Note that this method performs a binary search, hence it has as complexity of O(log n).
     *  If matchExact is false, and we don't have an exact match for the given msgId, we return the visible line right
     *  above the requested one.
     *  \param msgId      The message ID to look for
     *  \param matchExact Whether we find only exact matches
     *  \param ignoreDayChange Whether we ignore day change messages
     *  \return The ChatLine corresponding to the given MsgId
     */
    ChatLine *chatLine(MsgId msgId, bool matchExact = true, bool ignoreDayChange = true) const;

    inline ChatLine *lastLine() const { return _lines.count() ? _lines.last() : 0; }

    inline MarkerLineItem *markerLine() const { return _markerLine; }

    inline bool isSingleBufferScene() const { return _singleBufferId.isValid(); }
    inline BufferId singleBufferId() const { return _singleBufferId; }
    bool containsBuffer(const BufferId &id) const;

    ColumnHandleItem *firstColumnHandle() const;
    ColumnHandleItem *secondColumnHandle() const;

    inline CutoffMode senderCutoffMode() const { return _cutoffMode; }
    inline void setSenderCutoffMode(CutoffMode mode) { _cutoffMode = mode; }

    /**
     * Gets whether to re-add hidden brackets around sender for all message types
     *
     * Used within the Chat Monitor as the normal message prefixes are overridden.
     *
     * @return Whether to re-add hidden brackets around sender for all message types
     */
    inline bool alwaysBracketSender() const { return _alwaysBracketSender; }
    /**
     * Sets whether to re-add hidden brackets around sender for all message types
     *
     * @see ChatScene::alwaysBracketSender()
     *
     * @param brackets Sets whether to re-add hidden brackets around sender for all message types
     */
    inline void setAlwaysBracketSender(bool alwaysBracket) { _alwaysBracketSender = alwaysBracket; }

    QString selection() const;
    bool hasSelection() const;
    bool hasGlobalSelection() const;
    bool isPosOverSelection(const QPointF &) const;
    bool isGloballySelecting() const;
    void initiateDrag(QWidget *source);

    bool isScrollingAllowed() const;

public slots:
    void updateForViewport(qreal width, qreal height);
    void setWidth(qreal width);
    void layout(int start, int end, qreal width);

    void resetColumnWidths();

    void setMarkerLineVisible(bool visible = true);
    void setMarkerLine(MsgId msgId = MsgId());
    void jumpToMarkerLine(bool requestBacklog);

    // these are used by the chatitems to notify the scene and manage selections
    void setSelectingItem(ChatItem *item);
    ChatItem *selectingItem() const { return _selectingItem; }
    void startGlobalSelection(ChatItem *item, const QPointF &itemPos);
    void clearGlobalSelection();
    void clearSelection();
    void selectionToClipboard(QClipboard::Mode = QClipboard::Clipboard);
    void stringToClipboard(const QString &str, QClipboard::Mode = QClipboard::Clipboard);

    void webSearchOnSelection();

    void requestBacklog();

#if defined HAVE_WEBKIT || defined HAVE_WEBENGINE
    void loadWebPreview(ChatItem *parentItem, const QUrl &url, const QRectF &urlRect);
    void clearWebPreview(ChatItem *parentItem = 0);
#endif

signals:
    void lastLineChanged(QGraphicsItem *item, qreal offset);
    void layoutChanged(); // indicates changes to the scenerect due to resizing of the contentsitems
    void mouseMoveWhileSelecting(const QPointF &scenePos);

protected:
    virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent *contextMenuEvent);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent);
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent);
    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *mouseEvent);
    virtual void handleClick(Qt::MouseButton button, const QPointF &scenePos);

protected slots:
    void rowsInserted(const QModelIndex &, int, int);
    void rowsAboutToBeRemoved(const QModelIndex &, int, int);
    void dataChanged(const QModelIndex &, const QModelIndex &);

private slots:
    void firstHandlePositionChanged(qreal xpos);
    void secondHandlePositionChanged(qreal xpos);
#if defined HAVE_WEBKIT || defined HAVE_WEBENGINE
    void webPreviewNextStep();
#endif
    void showWebPreviewChanged();

    /**
     * Updates the local setting cache of whether or not to show sender brackets
     */
    void showSenderBracketsChanged();

    /**
     * Updates the local setting cache of whether or not to use the custom timestamp format
     */
    void useCustomTimestampFormatChanged();

    /**
     * Updates the local setting cache of the timestamp format string
     */
    void timestampFormatStringChanged();

    /**
     * Updates the status of whether or not the timestamp format string contains brackets
     *
     * When the timestamp contains brackets -and- showSenderBrackets is disabled, we need to
     * automatically add brackets.  This function checks if the timestamp has brackets and stores
     * the result, rather than checking each time text is copied.
     */
    void updateTimestampHasBrackets();

    void rowsRemoved();

    void clickTimeout();

private:
    void setHandleXLimits();
    void updateSelection(const QPointF &pos);

    ChatView *_chatView;
    QString _idString;
    QAbstractItemModel *_model;
    QList<ChatLine *> _lines;
    BufferId _singleBufferId;

    // calls to QChatScene::sceneRect() are very expensive. As we manage the scenerect ourselves
    // we store the size in a member variable.
    QRectF _sceneRect;
    int _firstLineRow; // the first row to display (aka: not a daychange msg)
    void updateSceneRect(qreal width);
    inline void updateSceneRect() { updateSceneRect(_sceneRect.width()); }
    void updateSceneRect(const QRectF &rect);
    qreal _viewportHeight;

    MarkerLineItem *_markerLine;
    bool _markerLineVisible, _markerLineValid, _markerLineJumpPending;

    ColumnHandleItem *_firstColHandle, *_secondColHandle;
    qreal _firstColHandlePos, _secondColHandlePos;
    int _defaultFirstColHandlePos, _defaultSecondColHandlePos;
    CutoffMode _cutoffMode;
    /// Whether to re-add hidden brackets around sender for all message types
    bool _alwaysBracketSender;

    ChatItem *_selectingItem;
    int _selectionStartCol, _selectionMinCol;
    int _selectionStart;
    int _selectionEnd;
    int _firstSelectionRow;
    bool _isSelecting;

    QTimer _clickTimer;
    ClickMode _clickMode;
    QPointF _clickPos;
    bool _clickHandled;
    bool _leftButtonPressed;

    bool _showWebPreview;

    bool _showSenderBrackets;  /// If true, show brackets around sender names

    bool _useCustomTimestampFormat; /// If true, use the custom timestamp format
    QString _timestampFormatString; /// Format of the timestamp string
    bool _timestampHasBrackets;     /// If true, timestamp format has [brackets] of some sort

    static const int _webSearchSelectionTextMaxVisible = 24;

#if defined HAVE_WEBKIT || defined HAVE_WEBENGINE
    struct WebPreview {
        enum PreviewState {
            NoPreview,
            NewPreview,
            DelayPreview,
            ShowPreview,
            HidePreview
        };
        ChatItem *parentItem;
        QGraphicsItem *previewItem;
        QUrl url;
        QRectF urlRect;
        PreviewState previewState;
        QTimer timer;
        WebPreview() : parentItem(0), previewItem(0), previewState(NoPreview) {}
    };
    WebPreview webPreview;
#endif // HAVE_WEBKIT || HAVE_WEBENGINE
};


#endif
