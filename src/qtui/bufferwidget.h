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

#pragma once

#include "abstractbuffercontainer.h"

#include "ui_bufferwidget.h"

class QGraphicsItem;
class ChatView;
class ChatViewSearchBar;
class ChatViewSearchController;

class BufferWidget : public AbstractBufferContainer
{
    Q_OBJECT

public:
    BufferWidget(QWidget* parent);
    ~BufferWidget() override;

    bool eventFilter(QObject* watched, QEvent* event) override;

    inline ChatViewSearchBar* searchBar() const { return ui.searchBar; }
    void addActionsToMenu(QMenu*, const QPointF& pos);
    virtual inline bool autoMarkerLineOnLostFocus() const { return _autoMarkerLineOnLostFocus; }

public slots:
    virtual void setMarkerLine(ChatView* view = nullptr, bool allowGoingBack = true);
    virtual void jumpToMarkerLine(ChatView* view = nullptr, bool requestBacklog = true);

protected:
    AbstractChatView* createChatView(BufferId) override;
    void removeChatView(BufferId) override;
    inline bool autoMarkerLine() const override { return _autoMarkerLine; }

protected slots:
    void currentChanged(const QModelIndex& current, const QModelIndex& previous) override;
    void showChatView(BufferId) override;

private slots:
    void scrollToHighlight(QGraphicsItem* highlightItem);
    void zoomIn();
    void zoomOut();
    void zoomOriginal();

    void setAutoMarkerLine(const QVariant&);
    void setAutoMarkerLineOnLostFocus(const QVariant&);
    /**
     * Sets the local cache of whether or not a buffer should fetch backlog upon show to provide a
     * scrollable amount of backlog
     *
     * @seealso BacklogSettings::setEnsureBacklogOnBufferShow()
     */
    void setEnsureBacklogOnBufferShow(const QVariant&);

private:
    Ui::BufferWidget ui;
    QHash<BufferId, QWidget*> _chatViews;

    ChatViewSearchController* _chatViewSearchController;

    bool _autoMarkerLine;
    bool _autoMarkerLineOnLostFocus;
    bool _ensureBacklogOnBufferShow; ///< If a buffer fetches backlog upon show until scrollable
};
