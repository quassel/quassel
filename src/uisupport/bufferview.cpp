/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
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

#include "bufferview.h"

#include <QApplication>
#include <QAction>
#include <QFlags>
#include <QHeaderView>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QSet>
#include <QVBoxLayout>

#include "action.h"
#include "buffermodel.h"
#include "bufferviewfilter.h"
#include "buffersettings.h"
#include "buffersyncer.h"
#include "client.h"
#include "contextmenuactionprovider.h"
#include "graphicalui.h"
#include "network.h"
#include "networkmodel.h"
#include "contextmenuactionprovider.h"

/*****************************************
* The TreeView showing the Buffers
*****************************************/
// Please be carefull when reimplementing methods which are used to inform the view about changes to the data
// to be on the safe side: call QTreeView's method aswell (or TreeViewTouch's)
BufferView::BufferView(QWidget *parent)
    : TreeViewTouch(parent)
{
    connect(this, SIGNAL(collapsed(const QModelIndex &)), SLOT(storeExpandedState(const QModelIndex &)));
    connect(this, SIGNAL(expanded(const QModelIndex &)), SLOT(storeExpandedState(const QModelIndex &)));

    setSelectionMode(QAbstractItemView::ExtendedSelection);

    QAbstractItemDelegate *oldDelegate = itemDelegate();
    BufferViewDelegate *tristateDelegate = new BufferViewDelegate(this);
    setItemDelegate(tristateDelegate);
    delete oldDelegate;
}


void BufferView::init()
{
    header()->setContextMenuPolicy(Qt::ActionsContextMenu);
    hideColumn(1);
    hideColumn(2);
    setIndentation(10);

    // New entries will be expanded automatically when added; no need to call expandAll()

    header()->hide(); // nobody seems to use this anyway

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // breaks with Qt 4.8
    if (QString("4.8.0") > qVersion()) // FIXME breaks with Qt versions >= 4.10!
        setAnimated(true);

    // FIXME This is to workaround bug #663
    setUniformRowHeights(true);

#ifndef QT_NO_DRAGANDDROP
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
#endif

    setSortingEnabled(true);
    sortByColumn(0, Qt::AscendingOrder);

#if defined Q_OS_MACOS || defined Q_OS_WIN
    // afaik this is better on Mac and Windows
    disconnect(this, SIGNAL(activated(QModelIndex)), this, SLOT(joinChannel(QModelIndex)));
    connect(this, SIGNAL(activated(QModelIndex)), SLOT(joinChannel(QModelIndex)));
#else
    disconnect(this, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(joinChannel(QModelIndex)));
    connect(this, SIGNAL(doubleClicked(QModelIndex)), SLOT(joinChannel(QModelIndex)));
#endif
}


void BufferView::setModel(QAbstractItemModel *model)
{
    delete selectionModel();

    TreeViewTouch::setModel(model);
    init();
    // remove old Actions
    QList<QAction *> oldactions = header()->actions();
    foreach(QAction *action, oldactions) {
        header()->removeAction(action);
        action->deleteLater();
    }

    if (!model)
        return;

    QString sectionName;
    QAction *showSection;
    for (int i = 1; i < model->columnCount(); i++) {
        sectionName = (model->headerData(i, Qt::Horizontal, Qt::DisplayRole)).toString();
        showSection = new QAction(sectionName, header());
        showSection->setCheckable(true);
        showSection->setChecked(!isColumnHidden(i));
        showSection->setProperty("column", i);
        connect(showSection, SIGNAL(toggled(bool)), this, SLOT(toggleHeader(bool)));
        header()->addAction(showSection);
    }

    connect(model, SIGNAL(layoutChanged()), this, SLOT(on_layoutChanged()));

    // Make sure collapsation is correct after setting a model
    // This might not be needed here, only in BufferView::setFilteredModel().  If issues arise, just
    // move down to setFilteredModel (which calls this function).
    setExpandedState();
}


void BufferView::setFilteredModel(QAbstractItemModel *model_, BufferViewConfig *config)
{
    BufferViewFilter *filter = qobject_cast<BufferViewFilter *>(model());
    if (filter) {
        filter->setConfig(config);
        setConfig(config);
        return;
    }

    if (model()) {
        disconnect(this, 0, model(), 0);
        disconnect(model(), 0, this, 0);
    }

    if (!model_) {
        setModel(model_);
    }
    else {
        BufferViewFilter *filter = new BufferViewFilter(model_, config);
        setModel(filter);
        connect(filter, SIGNAL(configChanged()), this, SLOT(on_configChanged()));
    }
    setConfig(config);
}


void BufferView::setConfig(BufferViewConfig *config)
{
    if (_config == config)
        return;

    if (_config) {
        disconnect(_config, 0, this, 0);
    }

    _config = config;
    if (config) {
        connect(config, SIGNAL(networkIdSet(const NetworkId &)), this, SLOT(setRootIndexForNetworkId(const NetworkId &)));
        setRootIndexForNetworkId(config->networkId());
    }
    else {
        setIndentation(10);
        setRootIndex(QModelIndex());
    }
}


void BufferView::setRootIndexForNetworkId(const NetworkId &networkId)
{
    if (!networkId.isValid() || !model()) {
        setIndentation(10);
        setRootIndex(QModelIndex());
    }
    else {
        setIndentation(5);
        int networkCount = model()->rowCount();
        QModelIndex child;
        for (int i = 0; i < networkCount; i++) {
            child = model()->index(i, 0);
            if (networkId == model()->data(child, NetworkModel::NetworkIdRole).value<NetworkId>())
                setRootIndex(child);
        }
    }
}


void BufferView::joinChannel(const QModelIndex &index)
{
    BufferInfo::Type bufferType = (BufferInfo::Type)index.data(NetworkModel::BufferTypeRole).value<int>();

    if (bufferType != BufferInfo::ChannelBuffer)
        return;

    BufferInfo bufferInfo = index.data(NetworkModel::BufferInfoRole).value<BufferInfo>();

    Client::userInput(bufferInfo, QString("/JOIN %1").arg(bufferInfo.bufferName()));
}


void BufferView::dropEvent(QDropEvent *event)
{
    QModelIndex index = indexAt(event->pos());

    QRect indexRect = visualRect(index);
    QPoint cursorPos = event->pos();

    // check if we're really _on_ the item and not indicating a move to just above or below the item
    // Magic margin number for this is from QAbstractItemViewPrivate::position()
    const int margin = 2;
    if (cursorPos.y() - indexRect.top() < margin
        || indexRect.bottom() - cursorPos.y() < margin)
        return TreeViewTouch::dropEvent(event);

    // If more than one buffer was being dragged, treat this as a rearrangement instead of a merge request
    QList<QPair<NetworkId, BufferId> > bufferList = Client::networkModel()->mimeDataToBufferList(event->mimeData());
    if (bufferList.count() != 1)
        return TreeViewTouch::dropEvent(event);

    // Get the Buffer ID of the buffer that was being dragged
    BufferId bufferId2 = bufferList[0].second;

    // Get the Buffer ID of the target buffer
    BufferId bufferId1 = index.data(NetworkModel::BufferIdRole).value<BufferId>();

    // If the source and target are the same buffer, this was an aborted rearrangement
    if (bufferId1 == bufferId2)
        return TreeViewTouch::dropEvent(event);

    // Get index of buffer that was being dragged
    QModelIndex index2 = Client::networkModel()->bufferIndex(bufferId2);

    // If the buffer being dragged is a channel and we're still joined to it, treat this as a rearrangement
    // This prevents us from being joined to a channel with no associated UI elements
    if (index2.data(NetworkModel::BufferTypeRole) == BufferInfo::ChannelBuffer && index2.data(NetworkModel::ItemActiveRole) == true)
        return TreeViewTouch::dropEvent(event);

    //If the source buffer is not mergeable(AKA not a Channel and not a Query), try rearranging instead
    if (index2.data(NetworkModel::BufferTypeRole) != BufferInfo::ChannelBuffer && index2.data(NetworkModel::BufferTypeRole) != BufferInfo::QueryBuffer)
        return TreeViewTouch::dropEvent(event);

    // If the target buffer is not mergeable(AKA not a Channel and not a Query), try rearranging instead
    if (index.data(NetworkModel::BufferTypeRole) != BufferInfo::ChannelBuffer && index.data(NetworkModel::BufferTypeRole) != BufferInfo::QueryBuffer)
        return TreeViewTouch::dropEvent(event);

    // Confirm that the user really wants to merge the buffers before doing so
    int res = QMessageBox::question(0, tr("Merge buffers permanently?"),
        tr("Do you want to merge the buffer \"%1\" permanently into buffer \"%2\"?\n This cannot be reversed!").arg(Client::networkModel()->bufferName(bufferId2)).arg(Client::networkModel()->bufferName(bufferId1)),
        QMessageBox::Yes|QMessageBox::No, QMessageBox::No);
    if (res == QMessageBox::Yes) {
        Client::mergeBuffersPermanently(bufferId1, bufferId2);
    }
}


void BufferView::removeSelectedBuffers(bool permanently)
{
    if (!config())
        return;

    BufferId bufferId;
    QSet<BufferId> removedRows;
    foreach(QModelIndex index, selectionModel()->selectedIndexes()) {
        if (index.data(NetworkModel::ItemTypeRole) != NetworkModel::BufferItemType)
            continue;

        bufferId = index.data(NetworkModel::BufferIdRole).value<BufferId>();
        if (removedRows.contains(bufferId))
            continue;

        removedRows << bufferId;
    }

    foreach(BufferId bufferId, removedRows) {
        if (permanently)
            config()->requestRemoveBufferPermanently(bufferId);
        else
            config()->requestRemoveBuffer(bufferId);
    }
}


void BufferView::rowsInserted(const QModelIndex &parent, int start, int end)
{
    TreeViewTouch::rowsInserted(parent, start, end);

    // ensure that newly inserted network nodes are expanded per default
    if (parent.data(NetworkModel::ItemTypeRole) != NetworkModel::NetworkItemType)
        return;

    setExpandedState(parent);
}


void BufferView::on_layoutChanged()
{
    int numNets = model()->rowCount(QModelIndex());
    for (int row = 0; row < numNets; row++) {
        QModelIndex networkIdx = model()->index(row, 0, QModelIndex());
        setExpandedState(networkIdx);
    }
}


void BufferView::on_configChanged()
{
    Q_ASSERT(model());

    // Expand/collapse as needed
    setExpandedState();

    if (config()) {
        // update selection to current one
        Client::bufferModel()->synchronizeView(this);
    }
}


void BufferView::setExpandedState()
{
    // Expand all active networks, collapse inactive ones... unless manually changed
    QModelIndex networkIdx;
    NetworkId networkId;
    for (int row = 0; row < model()->rowCount(); row++) {
        networkIdx = model()->index(row, 0);
        if (model()->rowCount(networkIdx) ==  0)
            continue;

        networkId = model()->data(networkIdx, NetworkModel::NetworkIdRole).value<NetworkId>();
        if (!networkId.isValid())
            continue;

        setExpandedState(networkIdx);
    }
}


void BufferView::storeExpandedState(const QModelIndex &networkIdx)
{
    NetworkId networkId = model()->data(networkIdx, NetworkModel::NetworkIdRole).value<NetworkId>();

    int oldState = 0;
    if (isExpanded(networkIdx))
        oldState |= WasExpanded;
    if (model()->data(networkIdx, NetworkModel::ItemActiveRole).toBool())
        oldState |= WasActive;

    _expandedState[networkId] = oldState;
}


void BufferView::setExpandedState(const QModelIndex &networkIdx)
{
    if (model()->data(networkIdx, NetworkModel::ItemTypeRole) != NetworkModel::NetworkItemType)
        return;

    if (model()->rowCount(networkIdx) == 0)
        return;

    NetworkId networkId = model()->data(networkIdx, NetworkModel::NetworkIdRole).value<NetworkId>();

    bool networkActive = model()->data(networkIdx, NetworkModel::ItemActiveRole).toBool();
    bool expandNetwork = networkActive;
    if (_expandedState.contains(networkId)) {
        int oldState = _expandedState[networkId];
        if ((bool)(oldState & WasActive) == networkActive)
            expandNetwork = (bool)(oldState & WasExpanded);
    }

    if (expandNetwork != isExpanded(networkIdx)) {
        update(networkIdx);
        setExpanded(networkIdx, expandNetwork);
    }
    storeExpandedState(networkIdx); // this call is needed to keep track of the isActive state
}

#if QT_VERSION < 0x050000
void BufferView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    TreeViewTouch::dataChanged(topLeft, bottomRight);
#else
void BufferView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    TreeViewTouch::dataChanged(topLeft, bottomRight, roles);
#endif

    // determine how many items have been changed and if any of them is a networkitem
    // which just swichted from active to inactive or vice versa
    if (topLeft.data(NetworkModel::ItemTypeRole) != NetworkModel::NetworkItemType)
        return;

    for (int i = topLeft.row(); i <= bottomRight.row(); i++) {
        QModelIndex networkIdx = topLeft.sibling(i, 0);
        setExpandedState(networkIdx);
    }
}


void BufferView::toggleHeader(bool checked)
{
    QAction *action = qobject_cast<QAction *>(sender());
    header()->setSectionHidden((action->property("column")).toInt(), !checked);
}


void BufferView::contextMenuEvent(QContextMenuEvent *event)
{
    QModelIndex index = indexAt(event->pos());
    if (!index.isValid())
        index = rootIndex();

    QMenu contextMenu(this);

    if (index.isValid()) {
        addActionsToMenu(&contextMenu, index);
    }

    addFilterActions(&contextMenu, index);

    if (!contextMenu.actions().isEmpty())
        contextMenu.exec(QCursor::pos());
}


void BufferView::addActionsToMenu(QMenu *contextMenu, const QModelIndex &index)
{
    QModelIndexList indexList = selectedIndexes();
    // make sure the item we clicked on is first
    indexList.removeAll(index);
    indexList.prepend(index);

    GraphicalUi::contextMenuActionProvider()->addActions(contextMenu, indexList, this, "menuActionTriggered", (bool)config());
}


void BufferView::addFilterActions(QMenu *contextMenu, const QModelIndex &index)
{
    BufferViewFilter *filter = qobject_cast<BufferViewFilter *>(model());
    if (filter) {
        QList<QAction *> filterActions = filter->actions(index);
        if (!filterActions.isEmpty()) {
            contextMenu->addSeparator();
            foreach(QAction *action, filterActions) {
                contextMenu->addAction(action);
            }
        }
    }
}


void BufferView::menuActionTriggered(QAction *result)
{
    ContextMenuActionProvider::ActionType type = (ContextMenuActionProvider::ActionType)result->data().toInt();
    switch (type) {
    case ContextMenuActionProvider::HideBufferTemporarily:
        removeSelectedBuffers();
        break;
    case ContextMenuActionProvider::HideBufferPermanently:
        removeSelectedBuffers(true);
        break;
    default:
        return;
    }
}


void BufferView::nextBuffer()
{
    changeBuffer(Forward);
}


void BufferView::previousBuffer()
{
    changeBuffer(Backward);
}


void BufferView::changeBuffer(Direction direction)
{
    QModelIndex currentIndex = selectionModel()->currentIndex();
    QModelIndex resultingIndex;

    QModelIndex lastNetIndex = model()->index(model()->rowCount() - 1, 0, QModelIndex());

    if (currentIndex.parent().isValid()) {
        //If we are a child node just switch among siblings unless it's the first/last child
        resultingIndex = currentIndex.sibling(currentIndex.row() + direction, 0);

        if (!resultingIndex.isValid()) {
            QModelIndex parent = currentIndex.parent();
            if (direction == Backward)
                resultingIndex = parent;
            else
                resultingIndex = parent.sibling(parent.row() + direction, 0);
        }
    }
    else {
        //If we have a toplevel node, try and get an adjacent child
        if (direction == Backward) {
            QModelIndex newParent = currentIndex.sibling(currentIndex.row() - 1, 0);
            if (currentIndex.row() == 0)
                newParent = lastNetIndex;
            if (model()->hasChildren(newParent))
                resultingIndex = newParent.child(model()->rowCount(newParent) - 1, 0);
            else
                resultingIndex = newParent;
        }
        else {
            if (model()->hasChildren(currentIndex))
                resultingIndex = currentIndex.child(0, 0);
            else
                resultingIndex = currentIndex.sibling(currentIndex.row() + 1, 0);
        }
    }

    if (!resultingIndex.isValid()) {
        if (direction == Forward)
            resultingIndex = model()->index(0, 0, QModelIndex());
        else
            resultingIndex = lastNetIndex.child(model()->rowCount(lastNetIndex) - 1, 0);
    }

    selectionModel()->setCurrentIndex(resultingIndex, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    selectionModel()->select(resultingIndex, QItemSelectionModel::ClearAndSelect);
}

void BufferView::selectFirstBuffer()
{
    int networksCount = model()->rowCount(QModelIndex());
    if (networksCount == 0) {
        return;
    }

    QModelIndex bufferIndex;
    for (int row = 0; row < networksCount; row++) {
        QModelIndex networkIndex = model()->index(row, 0, QModelIndex());
        int childCount = model()->rowCount(networkIndex);
        if (childCount > 0) {
            bufferIndex = model()->index(0, 0, networkIndex);
            break;
        }
    }

    if (!bufferIndex.isValid()) {
        return;
    }

    selectionModel()->setCurrentIndex(bufferIndex, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    selectionModel()->select(bufferIndex, QItemSelectionModel::ClearAndSelect);
}

void BufferView::wheelEvent(QWheelEvent *event)
{
    if (ItemViewSettings().mouseWheelChangesBuffer() == (bool)(event->modifiers() & Qt::AltModifier))
        return TreeViewTouch::wheelEvent(event);

    int rowDelta = (event->delta() > 0) ? -1 : 1;
    changeBuffer((Direction)rowDelta);
}


void BufferView::hideCurrentBuffer()
{
    QModelIndex index = selectionModel()->currentIndex();
    if (index.data(NetworkModel::ItemTypeRole) != NetworkModel::BufferItemType)
        return;

    BufferId bufferId = index.data(NetworkModel::BufferIdRole).value<BufferId>();

    //The check above means we won't be looking at a network, which should always be the first row, so we can just go backwards.
    changeBuffer(Backward);

    config()->requestRemoveBuffer(bufferId);
}


void BufferView::filterTextChanged(const QString& filterString)
{
    BufferViewFilter *filter = qobject_cast<BufferViewFilter *>(model());
    if (!filter) {
        return;
    }
    filter->setFilterString(filterString);
    on_configChanged(); // make sure collapsation is correct
}


void BufferView::changeHighlight(BufferView::Direction direction)
{
    // If for some weird reason we get a new delegate
    BufferViewDelegate *delegate = qobject_cast<BufferViewDelegate*>(itemDelegate(_currentHighlight));
    if (delegate) {
        delegate->currentHighlight = QModelIndex();
    }

    QModelIndex newIndex = _currentHighlight;
    if (!newIndex.isValid()) {
        newIndex = model()->index(0, 0);
    }

    if (direction == Backward) {
        newIndex = indexBelow(newIndex);
    } else {
        newIndex = indexAbove(newIndex);
    }

    if (!newIndex.isValid()) {
        return;
    }

    _currentHighlight = newIndex;

    delegate = qobject_cast<BufferViewDelegate*>(itemDelegate(_currentHighlight));
    if (delegate) {
        delegate->currentHighlight = _currentHighlight;
    }
    viewport()->update();
}

void BufferView::selectHighlighted()
{
    if (_currentHighlight.isValid()) {
        selectionModel()->setCurrentIndex(_currentHighlight, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        selectionModel()->select(_currentHighlight, QItemSelectionModel::ClearAndSelect);
    } else {
        selectFirstBuffer();
    }

    clearHighlight();
}

void BufferView::clearHighlight()
{
    // If for some weird reason we get a new delegate
    BufferViewDelegate *delegate = qobject_cast<BufferViewDelegate*>(itemDelegate(_currentHighlight));
    if (delegate) {
        delegate->currentHighlight = QModelIndex();
    }
    _currentHighlight = QModelIndex();
    viewport()->update();
}

// ****************************************
//  BufferViewDelegate
// ****************************************
class ColorsChangedEvent : public QEvent
{
public:
    ColorsChangedEvent() : QEvent(QEvent::User) {};
};


BufferViewDelegate::BufferViewDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}


void BufferViewDelegate::customEvent(QEvent *event)
{
    if (event->type() != QEvent::User)
        return;

    event->accept();
}


bool BufferViewDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if (event->type() != QEvent::MouseButtonRelease)
        return QStyledItemDelegate::editorEvent(event, model, option, index);

    if (!(model->flags(index) & Qt::ItemIsUserCheckable))
        return QStyledItemDelegate::editorEvent(event, model, option, index);

    QVariant value = index.data(Qt::CheckStateRole);
    if (!value.isValid())
        return QStyledItemDelegate::editorEvent(event, model, option, index);

#if QT_VERSION < 0x050000
    QStyleOptionViewItemV4 viewOpt(option);
#else
    QStyleOptionViewItem viewOpt(option);
#endif
    initStyleOption(&viewOpt, index);

    QRect checkRect = viewOpt.widget->style()->subElementRect(QStyle::SE_ItemViewItemCheckIndicator, &viewOpt, viewOpt.widget);
    QMouseEvent *me = static_cast<QMouseEvent *>(event);

    if (me->button() != Qt::LeftButton || !checkRect.contains(me->pos()))
        return QStyledItemDelegate::editorEvent(event, model, option, index);

    Qt::CheckState state = static_cast<Qt::CheckState>(value.toInt());
    if (state == Qt::Unchecked)
        state = Qt::PartiallyChecked;
    else if (state == Qt::PartiallyChecked)
        state = Qt::Checked;
    else
        state = Qt::Unchecked;
    model->setData(index, state, Qt::CheckStateRole);
    return true;
}


// ==============================
//  BufferView Dock
// ==============================
BufferViewDock::BufferViewDock(BufferViewConfig *config, QWidget *parent)
    : QDockWidget(parent),
    _childWidget(0),
    _widget(new QWidget(parent)),
    _filterEdit(new QLineEdit(parent)),
    _active(false),
    _title(config->bufferViewName())
{
    setObjectName("BufferViewDock-" + QString::number(config->bufferViewId()));
    toggleViewAction()->setData(config->bufferViewId());
    setAllowedAreas(Qt::RightDockWidgetArea|Qt::LeftDockWidgetArea);
    connect(config, SIGNAL(bufferViewNameSet(const QString &)), this, SLOT(bufferViewRenamed(const QString &)));
    connect(config, SIGNAL(configChanged()), SLOT(configChanged()));
    updateTitle();

    _widget->setLayout(new QVBoxLayout);
    _widget->layout()->setSpacing(0);
    _widget->layout()->setContentsMargins(0, 0, 0, 0);

    // We need to potentially hide it early, so it doesn't flicker
    _filterEdit->setVisible(config->showSearch());
    _filterEdit->setFocusPolicy(Qt::ClickFocus);
    _filterEdit->installEventFilter(this);
    _filterEdit->setPlaceholderText(tr("Search..."));
    connect(_filterEdit, SIGNAL(returnPressed()), SLOT(onFilterReturnPressed()));

    _widget->layout()->addWidget(_filterEdit);
    QDockWidget::setWidget(_widget);
}

void BufferViewDock::setLocked(bool locked) {
    if (locked) {
        setFeatures(0);
    }
    else {
        setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    }
}

void BufferViewDock::updateTitle()
{
    QString title = _title;
    if (isActive())
        title.prepend(QString::fromUtf8("• "));
    setWindowTitle(title);
}

void BufferViewDock::configChanged()
{
    if (_filterEdit->isVisible() != config()->showSearch()) {
        _filterEdit->setVisible(config()->showSearch());
        _filterEdit->clear();
    }
}

void BufferViewDock::onFilterReturnPressed()
{
    if (_oldFocusItem) {
        _oldFocusItem->setFocus();
        _oldFocusItem = 0;
    }

    if (!config()->showSearch()) {
        _filterEdit->setVisible(false);
    }

    BufferView *view = bufferView();
    if (!view) {
        return;
    }

    if (!_filterEdit->text().isEmpty()) {
        view->selectHighlighted();
        _filterEdit->clear();
    } else {
        view->clearHighlight();
    }
}

void BufferViewDock::setActive(bool active)
{
    if (active != isActive()) {
        _active = active;
        updateTitle();
        if (active) {
            raise();  // for tabbed docks
        }
    }
}

bool BufferViewDock::eventFilter(QObject *object, QEvent *event)
{
   if (object != _filterEdit)  {
       return false;
   }

   if (event->type() == QEvent::FocusOut) {
       if (!config()->showSearch() && _filterEdit->text().isEmpty()) {
           _filterEdit->setVisible(false);
           return true;
       }
   } else if (event->type() == QEvent::KeyRelease) {
       QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

       BufferView *view = bufferView();
       if (!view) {
           return false;
       }

       switch (keyEvent->key()) {
       case Qt::Key_Escape: {
           _filterEdit->clear();

           if (!_oldFocusItem) {
               return false;
           }

           _oldFocusItem->setFocus();
           _oldFocusItem = nullptr;
           return true;
       }
       case Qt::Key_Down:
           view->changeHighlight(BufferView::Backward);
           return true;
       case Qt::Key_Up:
           view->changeHighlight(BufferView::Forward);
           return true;
       default:
           break;
       }

       return false;
   }

   return false;
}

void BufferViewDock::bufferViewRenamed(const QString &newName)
{
    _title = newName;
    updateTitle();
    toggleViewAction()->setText(newName);
}


int BufferViewDock::bufferViewId() const
{
    BufferView *view = bufferView();
    if (!view)
        return 0;

    if (view->config())
        return view->config()->bufferViewId();
    else
        return 0;
}


BufferViewConfig *BufferViewDock::config() const
{
    BufferView *view = bufferView();
    if (!view)
        return 0;
    else
        return view->config();
}

void BufferViewDock::setWidget(QWidget *newWidget)
{
    _widget->layout()->addWidget(newWidget);
    _childWidget = newWidget;

    connect(_filterEdit, SIGNAL(textChanged(QString)), bufferView(), SLOT(filterTextChanged(QString)));
}

void BufferViewDock::activateFilter()
{
    if (!_filterEdit->isVisible()) {
        _filterEdit->setVisible(true);
    }

    _oldFocusItem = qApp->focusWidget();

    _filterEdit->setFocus();
}


void BufferViewDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem newOption = option;
    if (index == currentHighlight) {
        newOption.state |= QStyle::State_HasFocus;
    }
    QStyledItemDelegate::paint(painter, newOption, index);
}
