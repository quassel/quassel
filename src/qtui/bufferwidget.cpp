/***************************************************************************
 *   Copyright (C) 2005-2013 by the Quassel Project                        *
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

#include <QLayout>
#include <QKeyEvent>
#include <QMenu>
#include <QScrollBar>

#include "action.h"
#include "actioncollection.h"
#include "bufferwidget.h"
#include "chatline.h"
#include "chatview.h"
#include "chatviewsearchbar.h"
#include "chatviewsearchcontroller.h"
#include "chatviewsettings.h"
#include "client.h"
#include "iconloader.h"
#include "multilineedit.h"
#include "qtui.h"
#include "settings.h"

BufferWidget::BufferWidget(QWidget *parent)
    : AbstractBufferContainer(parent),
    _chatViewSearchController(new ChatViewSearchController(this)),
    _autoMarkerLine(true)
{
    ui.setupUi(this);
    layout()->setContentsMargins(0, 0, 0, 0);
    layout()->setSpacing(0);
    // ui.searchBar->hide();

    _chatViewSearchController->setCaseSensitive(ui.searchBar->caseSensitiveBox()->isChecked());
    _chatViewSearchController->setSearchSenders(ui.searchBar->searchSendersBox()->isChecked());
    _chatViewSearchController->setSearchMsgs(ui.searchBar->searchMsgsBox()->isChecked());
    _chatViewSearchController->setSearchOnlyRegularMsgs(ui.searchBar->searchOnlyRegularMsgsBox()->isChecked());

    connect(ui.searchBar, SIGNAL(searchChanged(const QString &)),
        _chatViewSearchController, SLOT(setSearchString(const QString &)));
    connect(ui.searchBar->caseSensitiveBox(), SIGNAL(toggled(bool)),
        _chatViewSearchController, SLOT(setCaseSensitive(bool)));
    connect(ui.searchBar->searchSendersBox(), SIGNAL(toggled(bool)),
        _chatViewSearchController, SLOT(setSearchSenders(bool)));
    connect(ui.searchBar->searchMsgsBox(), SIGNAL(toggled(bool)),
        _chatViewSearchController, SLOT(setSearchMsgs(bool)));
    connect(ui.searchBar->searchOnlyRegularMsgsBox(), SIGNAL(toggled(bool)),
        _chatViewSearchController, SLOT(setSearchOnlyRegularMsgs(bool)));
    connect(ui.searchBar->searchUpButton(), SIGNAL(clicked()),
        _chatViewSearchController, SLOT(highlightPrev()));
    connect(ui.searchBar->searchDownButton(), SIGNAL(clicked()),
        _chatViewSearchController, SLOT(highlightNext()));

    connect(ui.searchBar, SIGNAL(hidden()), this, SLOT(setFocus()));

    connect(_chatViewSearchController, SIGNAL(newCurrentHighlight(QGraphicsItem *)),
        this, SLOT(scrollToHighlight(QGraphicsItem *)));

    ActionCollection *coll = QtUi::actionCollection();

    Action *zoomInChatview = coll->add<Action>("ZoomInChatView", this, SLOT(zoomIn()));
    zoomInChatview->setText(tr("Zoom In"));
    zoomInChatview->setIcon(SmallIcon("zoom-in"));
    zoomInChatview->setShortcut(QKeySequence::ZoomIn);

    Action *zoomOutChatview = coll->add<Action>("ZoomOutChatView", this, SLOT(zoomOut()));
    zoomOutChatview->setIcon(SmallIcon("zoom-out"));
    zoomOutChatview->setText(tr("Zoom Out"));
    zoomOutChatview->setShortcut(QKeySequence::ZoomOut);

    Action *zoomOriginalChatview = coll->add<Action>("ZoomOriginalChatView", this, SLOT(zoomOriginal()));
    zoomOriginalChatview->setIcon(SmallIcon("zoom-original"));
    zoomOriginalChatview->setText(tr("Actual Size"));
    //zoomOriginalChatview->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_0)); // used for RTS switching

    Action *setMarkerLine = coll->add<Action>("SetMarkerLineToBottom", this, SLOT(setMarkerLine()));
    setMarkerLine->setText(tr("Set Marker Line"));
    setMarkerLine->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_R));

    Action *jumpToMarkerLine = QtUi::actionCollection("Navigation")->add<Action>("JumpToMarkerLine", this, SLOT(jumpToMarkerLine()));
    jumpToMarkerLine->setText(tr("Go to Marker Line"));
    jumpToMarkerLine->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_K));

    ChatViewSettings s;
    s.initAndNotify("AutoMarkerLine", this, SLOT(setAutoMarkerLine(QVariant)), true);
}


BufferWidget::~BufferWidget()
{
    delete _chatViewSearchController;
    _chatViewSearchController = 0;
}


void BufferWidget::setAutoMarkerLine(const QVariant &v)
{
    _autoMarkerLine = v.toBool();
}


AbstractChatView *BufferWidget::createChatView(BufferId id)
{
    ChatView *chatView;
    chatView = new ChatView(id, this);
    chatView->setBufferContainer(this);
    _chatViews[id] = chatView;
    ui.stackedWidget->addWidget(chatView);
    chatView->setFocusProxy(this);
    return chatView;
}


void BufferWidget::removeChatView(BufferId id)
{
    QWidget *view = _chatViews.value(id, 0);
    if (!view) return;
    ui.stackedWidget->removeWidget(view);
    view->deleteLater();
    _chatViews.take(id);
}


void BufferWidget::showChatView(BufferId id)
{
    if (!id.isValid()) {
        ui.stackedWidget->setCurrentWidget(ui.page);
    }
    else {
        ChatView *view = qobject_cast<ChatView *>(_chatViews.value(id));
        Q_ASSERT(view);
        ui.stackedWidget->setCurrentWidget(view);
        _chatViewSearchController->setScene(view->scene());
    }
}


void BufferWidget::scrollToHighlight(QGraphicsItem *highlightItem)
{
    ChatView *view = qobject_cast<ChatView *>(ui.stackedWidget->currentWidget());
    if (view) {
        view->centerOn(highlightItem);
    }
}


void BufferWidget::zoomIn()
{
    ChatView *view = qobject_cast<ChatView *>(ui.stackedWidget->currentWidget());
    if (view)
        view->zoomIn();
}


void BufferWidget::zoomOut()
{
    ChatView *view = qobject_cast<ChatView *>(ui.stackedWidget->currentWidget());
    if (view)
        view->zoomOut();
}


void BufferWidget::zoomOriginal()
{
    ChatView *view = qobject_cast<ChatView *>(ui.stackedWidget->currentWidget());
    if (view)
        view->zoomOriginal();
}


void BufferWidget::addActionsToMenu(QMenu *menu, const QPointF &pos)
{
    Q_UNUSED(pos);
    ActionCollection *coll = QtUi::actionCollection();
    menu->addSeparator();
    menu->addAction(coll->action("ZoomInChatView"));
    menu->addAction(coll->action("ZoomOutChatView"));
    menu->addAction(coll->action("ZoomOriginalChatView"));
}


bool BufferWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() != QEvent::KeyPress)
        return false;

    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

    MultiLineEdit *inputLine = qobject_cast<MultiLineEdit *>(watched);
    if (!inputLine)
        return false;

    // Intercept copy key presses
    if (keyEvent == QKeySequence::Copy) {
        if (inputLine->hasSelectedText())
            return false;
        ChatView *view = qobject_cast<ChatView *>(ui.stackedWidget->currentWidget());
        if (view)
            view->scene()->selectionToClipboard();
        return true;
    }

    // We don't want to steal cursor movement keys if the input line is in multiline mode
    if (!inputLine->isSingleLine())
        return false;

    switch (keyEvent->key()) {
    case Qt::Key_Up:
    case Qt::Key_Down:
        if (!(keyEvent->modifiers() & Qt::ShiftModifier))
            return false;
    case Qt::Key_PageUp:
    case Qt::Key_PageDown:
        // static cast to access public qobject::event
        return static_cast<QObject *>(ui.stackedWidget->currentWidget())->event(event);
    default:
        return false;
    }
}


void BufferWidget::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    ChatView *prevView = qobject_cast<ChatView *>(ui.stackedWidget->currentWidget());

    AbstractBufferContainer::currentChanged(current, previous); // switch first to avoid a redraw

    // we need to hide the marker line if it's already/still at the bottom of the view (and not scrolled up)
    ChatView *curView = qobject_cast<ChatView *>(ui.stackedWidget->currentWidget());
    if (curView) {
        BufferId curBufferId = current.data(NetworkModel::BufferIdRole).value<BufferId>();
        if (curBufferId.isValid()) {
            MsgId markerMsgId = Client::networkModel()->markerLineMsgId(curBufferId);
            if (markerMsgId == curView->lastMsgId() && markerMsgId == curView->lastVisibleMsgId())
                curView->setMarkerLineVisible(false);
            else
                curView->setMarkerLineVisible(true);
        }
    }

    if (prevView && autoMarkerLine())
        setMarkerLine(prevView, false);
}


void BufferWidget::setMarkerLine(ChatView *view, bool allowGoingBack)
{
    if (!view)
        view = qobject_cast<ChatView *>(ui.stackedWidget->currentWidget());
    if (!view)
        return;

    ChatLine *lastLine = view->lastVisibleChatLine();
    if (lastLine) {
        QModelIndex idx = lastLine->index();
        MsgId msgId = idx.data(MessageModel::MsgIdRole).value<MsgId>();

        if (!allowGoingBack) {
            BufferId bufId = view->scene()->singleBufferId();
            MsgId oldMsgId = Client::markerLine(bufId);
            if (oldMsgId.isValid() && msgId <= oldMsgId)
                return;
        }
        view->setMarkerLine(msgId);
    }
}


void BufferWidget::jumpToMarkerLine(ChatView *view, bool requestBacklog)
{
    if (!view)
        view = qobject_cast<ChatView *>(ui.stackedWidget->currentWidget());
    if (!view)
        return;

    view->jumpToMarkerLine(requestBacklog);
}
