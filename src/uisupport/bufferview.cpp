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

#include "bufferview.h"

#include <QApplication>
#include <QAction>
#include <QFlags>
#include <QHeaderView>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QSet>

#include "action.h"
#include "buffermodel.h"
#include "bufferviewfilter.h"
#include "buffersettings.h"
#include "buffersyncer.h"
#include "client.h"
#include "iconloader.h"
#include "mappedselectionmodel.h"
#include "network.h"
#include "networkmodel.h"
#include "networkmodelactionprovider.h"
#include "quasselui.h"
#include "uisettings.h"

bool TristateDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) {
  if(event->type() != QEvent::MouseButtonRelease)
    return QStyledItemDelegate::editorEvent(event, model, option, index);

  if(!(model->flags(index) & Qt::ItemIsUserCheckable))
    return QStyledItemDelegate::editorEvent(event, model, option, index);

  QVariant value = index.data(Qt::CheckStateRole);
  if(!value.isValid())
    return QStyledItemDelegate::editorEvent(event, model, option, index);

  QStyleOptionViewItemV4 viewOpt(option);
  initStyleOption(&viewOpt, index);

  QRect checkRect = viewOpt.widget->style()->subElementRect(QStyle::SE_ItemViewItemCheckIndicator, &viewOpt, viewOpt.widget);
  QMouseEvent *me = static_cast<QMouseEvent*>(event);

  if(me->button() != Qt::LeftButton || !checkRect.contains(me->pos()))
    return QStyledItemDelegate::editorEvent(event, model, option, index);

  Qt::CheckState state = static_cast<Qt::CheckState>(value.toInt());
  if(state == Qt::Unchecked)
    state = Qt::PartiallyChecked;
  else if(state == Qt::PartiallyChecked)
    state = Qt::Checked;
  else
    state = Qt::Unchecked;
  model->setData(index, state, Qt::CheckStateRole);
  return true;
}




/*****************************************
* The TreeView showing the Buffers
*****************************************/
// Please be carefull when reimplementing methods which are used to inform the view about changes to the data
// to be on the safe side: call QTreeView's method aswell
BufferView::BufferView(QWidget *parent)
  : QTreeView(parent)
{
  connect(this, SIGNAL(collapsed(const QModelIndex &)), SLOT(on_collapse(const QModelIndex &)));
  connect(this, SIGNAL(expanded(const QModelIndex &)), SLOT(on_expand(const QModelIndex &)));

  setSelectionMode(QAbstractItemView::ExtendedSelection);

  QAbstractItemDelegate *oldDelegate = itemDelegate();
  TristateDelegate *tristateDelegate = new TristateDelegate(this);
  setItemDelegate(tristateDelegate);
  delete oldDelegate;
}

void BufferView::init() {
  header()->setContextMenuPolicy(Qt::ActionsContextMenu);
  hideColumn(1);
  hideColumn(2);
  setIndentation(10);
  expandAll();

  setAnimated(true);

#ifndef QT_NO_DRAGANDDROP
  setDragEnabled(true);
  setAcceptDrops(true);
  setDropIndicatorShown(true);
#endif

  setSortingEnabled(true);
  sortByColumn(0, Qt::AscendingOrder);

  // activated() fails on X11 and Qtopia at least
#if defined Q_WS_QWS || defined Q_WS_X11
  connect(this, SIGNAL(doubleClicked(QModelIndex)), SLOT(joinChannel(QModelIndex)));
#else
  // afaik this is better on Mac and Windows
  connect(this, SIGNAL(activated(QModelIndex)), SLOT(joinChannel(QModelIndex)));
#endif
}

void BufferView::setModel(QAbstractItemModel *model) {
  delete selectionModel();

  QTreeView::setModel(model);
  init();
  // remove old Actions
  QList<QAction *> oldactions = header()->actions();
  foreach(QAction *action, oldactions) {
    header()->removeAction(action);
    action->deleteLater();
  }

  if(!model)
    return;

  QString sectionName;
  QAction *showSection;
  for(int i = 1; i < model->columnCount(); i++) {
    sectionName = (model->headerData(i, Qt::Horizontal, Qt::DisplayRole)).toString();
    showSection = new QAction(sectionName, header());
    showSection->setCheckable(true);
    showSection->setChecked(!isColumnHidden(i));
    showSection->setProperty("column", i);
    connect(showSection, SIGNAL(toggled(bool)), this, SLOT(toggleHeader(bool)));
    header()->addAction(showSection);
  }

}

void BufferView::setFilteredModel(QAbstractItemModel *model_, BufferViewConfig *config) {
  BufferViewFilter *filter = qobject_cast<BufferViewFilter *>(model());
  if(filter) {
    filter->setConfig(config);
    setConfig(config);
    return;
  }

  if(model()) {
    disconnect(this, 0, model(), 0);
    disconnect(model(), 0, this, 0);
  }

  if(!model_) {
    setModel(model_);
  } else {
    BufferViewFilter *filter = new BufferViewFilter(model_, config);
    setModel(filter);
    connect(filter, SIGNAL(configChanged()), this, SLOT(on_configChanged()));
  }
  setConfig(config);
}

void BufferView::setSelectionModel(QItemSelectionModel *selectionModel) {
  if(QTreeView::selectionModel())
    disconnect(selectionModel, SIGNAL(currentChanged(QModelIndex, QModelIndex)),
               model(), SIGNAL(checkPreviousCurrentForRemoval(QModelIndex, QModelIndex)));

  QTreeView::setSelectionModel(selectionModel);
  BufferViewFilter *filter = qobject_cast<BufferViewFilter *>(model());
  if(filter) {
    connect(selectionModel, SIGNAL(currentChanged(QModelIndex, QModelIndex)),
            filter, SLOT(checkPreviousCurrentForRemoval(QModelIndex, QModelIndex)));
  }
}

void BufferView::setConfig(BufferViewConfig *config) {
  if(_config == config)
    return;

  if(_config) {
    disconnect(_config, 0, this, 0);
  }

  _config = config;
  if(config) {
    connect(config, SIGNAL(networkIdSet(const NetworkId &)), this, SLOT(setRootIndexForNetworkId(const NetworkId &)));
    setRootIndexForNetworkId(config->networkId());
  } else {
    setIndentation(10);
    setRootIndex(QModelIndex());
  }
}

void BufferView::setRootIndexForNetworkId(const NetworkId &networkId) {
  if(!networkId.isValid() || !model()) {
    setIndentation(10);
    setRootIndex(QModelIndex());
  } else {
    setIndentation(5);
    int networkCount = model()->rowCount();
    QModelIndex child;
    for(int i = 0; i < networkCount; i++) {
      child = model()->index(i, 0);
      if(networkId == model()->data(child, NetworkModel::NetworkIdRole).value<NetworkId>())
        setRootIndex(child);
    }
  }
}

void BufferView::joinChannel(const QModelIndex &index) {
  BufferInfo::Type bufferType = (BufferInfo::Type)index.data(NetworkModel::BufferTypeRole).value<int>();

  if(bufferType != BufferInfo::ChannelBuffer)
    return;

  BufferInfo bufferInfo = index.data(NetworkModel::BufferInfoRole).value<BufferInfo>();

  Client::userInput(bufferInfo, QString("/JOIN %1").arg(bufferInfo.bufferName()));
}

void BufferView::keyPressEvent(QKeyEvent *event) {
  if(event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete) {
    event->accept();
    removeSelectedBuffers();
  }
  QTreeView::keyPressEvent(event);
}

void BufferView::dropEvent(QDropEvent *event) {
  QModelIndex index = indexAt(event->pos());

  QRect indexRect = visualRect(index);
  QPoint cursorPos = event->pos();

  // check if we're really _on_ the item and not indicating a move to just above or below the item
  const int margin = 2;
  if(cursorPos.y() - indexRect.top() < margin
     || indexRect.bottom() - cursorPos.y() < margin)
    return QTreeView::dropEvent(event);

  QList< QPair<NetworkId, BufferId> > bufferList = Client::networkModel()->mimeDataToBufferList(event->mimeData());
  if(bufferList.count() != 1)
    return QTreeView::dropEvent(event);

  NetworkId networkId = bufferList[0].first;
  BufferId bufferId2 = bufferList[0].second;

  if(index.data(NetworkModel::ItemTypeRole) != NetworkModel::BufferItemType)
    return QTreeView::dropEvent(event);

  if(index.data(NetworkModel::BufferTypeRole) != BufferInfo::QueryBuffer)
    return QTreeView::dropEvent(event);

  if(index.data(NetworkModel::NetworkIdRole).value<NetworkId>() != networkId)
    return QTreeView::dropEvent(event);

  BufferId bufferId1 = index.data(NetworkModel::BufferIdRole).value<BufferId>();
  if(bufferId1 == bufferId2)
    return QTreeView::dropEvent(event);

  int res = QMessageBox::question(0, tr("Merge buffers permanently?"),
				  tr("Do you want to merge the buffer \"%1\" permanently into buffer \"%2\"?\n This cannot be reversed!").arg(Client::networkModel()->bufferName(bufferId2)).arg(Client::networkModel()->bufferName(bufferId1)),
				  QMessageBox::Yes|QMessageBox::No, QMessageBox::No);
  if(res == QMessageBox::Yes) {
    Client::mergeBuffersPermanently(bufferId1, bufferId2);
  }
}

void BufferView::removeSelectedBuffers(bool permanently) {
  if(!config())
    return;

  BufferId bufferId;
  QSet<BufferId> removedRows;
  foreach(QModelIndex index, selectionModel()->selectedIndexes()) {
    if(index.data(NetworkModel::ItemTypeRole) != NetworkModel::BufferItemType)
      continue;

    bufferId = index.data(NetworkModel::BufferIdRole).value<BufferId>();
    if(removedRows.contains(bufferId))
      continue;

    removedRows << bufferId;

    if(permanently)
      config()->requestRemoveBufferPermanently(bufferId);
    else
      config()->requestRemoveBuffer(bufferId);
  }
}

void BufferView::rowsInserted(const QModelIndex & parent, int start, int end) {
  QTreeView::rowsInserted(parent, start, end);

  // ensure that newly inserted network nodes are expanded per default
  if(parent.data(NetworkModel::ItemTypeRole) != NetworkModel::NetworkItemType)
    return;

  if(model()->rowCount(parent) == 1 && parent.data(NetworkModel::ItemActiveRole) == true) {
    // without updating the parent the expand will have no effect... Qt Bug?
    update(parent);
    expand(parent);
  }
}

void BufferView::on_configChanged() {
  Q_ASSERT(model());

  // expand all active networks... collapse inactive ones... unless manually changed
  QModelIndex networkIdx;
  NetworkId networkId;
  for(int row = 0; row < model()->rowCount(); row++) {
    networkIdx = model()->index(row, 0);
    if(model()->rowCount(networkIdx) ==  0)
      continue;

    networkId = model()->data(networkIdx, NetworkModel::NetworkIdRole).value<NetworkId>();
    if(!networkId.isValid())
      continue;

    update(networkIdx);

    bool expandNetwork = false;
    if(_expandedState.contains(networkId))
      expandNetwork = _expandedState[networkId];
    else
      expandNetwork = model()->data(networkIdx, NetworkModel::ItemActiveRole).toBool();

    if(expandNetwork)
      expand(networkIdx);
    else
      collapse(networkIdx);
  }

  // update selection to current one
  MappedSelectionModel *mappedSelectionModel = qobject_cast<MappedSelectionModel *>(selectionModel());
  if(!config() || !mappedSelectionModel)
    return;

  mappedSelectionModel->mappedSetCurrentIndex(Client::bufferModel()->standardSelectionModel()->currentIndex(), QItemSelectionModel::Current);
  mappedSelectionModel->mappedSelect(Client::bufferModel()->standardSelectionModel()->selection(), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}

void BufferView::on_collapse(const QModelIndex &index) {
  storeExpandedState(index.data(NetworkModel::NetworkIdRole).value<NetworkId>(), false);
}

void BufferView::on_expand(const QModelIndex &index) {
  storeExpandedState(index.data(NetworkModel::NetworkIdRole).value<NetworkId>(), true);
}

void BufferView::storeExpandedState(NetworkId networkId, bool expanded) {
  _expandedState[networkId] = expanded;
}

void BufferView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight) {
  QTreeView::dataChanged(topLeft, bottomRight);

  // determine how many items have been changed and if any of them is a networkitem
  // which just swichted from active to inactive or vice versa
  if(topLeft.data(NetworkModel::ItemTypeRole) != NetworkModel::NetworkItemType)
    return;

  for(int i = topLeft.row(); i <= bottomRight.row(); i++) {
    QModelIndex networkIdx = topLeft.sibling(i, 0);
    if(model()->rowCount(networkIdx) == 0)
      continue;

    bool isActive = networkIdx.data(NetworkModel::ItemActiveRole).toBool();
#ifdef SPUTDEV
    if(isExpanded(networkIdx) != isActive) setExpanded(networkIdx, true);
#else
    if(isExpanded(networkIdx) != isActive) setExpanded(networkIdx, isActive);
#endif
  }
}

void BufferView::toggleHeader(bool checked) {
  QAction *action = qobject_cast<QAction *>(sender());
  header()->setSectionHidden((action->property("column")).toInt(), !checked);
}

void BufferView::contextMenuEvent(QContextMenuEvent *event) {
  QModelIndex index = indexAt(event->pos());
  if(!index.isValid())
    index = rootIndex();

  QMenu contextMenu(this);

  if(index.isValid()) {
    addActionsToMenu(&contextMenu, index);
  }

  addFilterActions(&contextMenu, index);

  if(!contextMenu.actions().isEmpty())
    contextMenu.exec(QCursor::pos());
}

void BufferView::addActionsToMenu(QMenu *contextMenu, const QModelIndex &index) {
  Client::mainUi()->actionProvider()->addActions(contextMenu, index, this, "menuActionTriggered", (bool)config());
}

void BufferView::addFilterActions(QMenu *contextMenu, const QModelIndex &index) {
  BufferViewFilter *filter = qobject_cast<BufferViewFilter *>(model());
  if(filter) {
    QList<QAction *> filterActions = filter->actions(index);
    if(!filterActions.isEmpty()) {
      contextMenu->addSeparator();
      foreach(QAction *action, filterActions) {
	contextMenu->addAction(action);
      }
    }
  }
}

void BufferView::menuActionTriggered(QAction *result) {
  NetworkModelActionProvider::ActionType type = (NetworkModelActionProvider::ActionType)result->data().toInt();
  switch(type) {
    case NetworkModelActionProvider::HideBufferTemporarily:
      removeSelectedBuffers();
      break;
    case NetworkModelActionProvider::HideBufferPermanently:
      removeSelectedBuffers(true);
      break;
    default:
      return;
  }
}

void BufferView::wheelEvent(QWheelEvent* event) {
  if(UiSettings().value("MouseWheelChangesBuffers", QVariant(true)).toBool() == (bool)(event->modifiers() & Qt::AltModifier))
    return QTreeView::wheelEvent(event);

  int rowDelta = ( event->delta() > 0 ) ? -1 : 1;
  QModelIndex currentIndex = selectionModel()->currentIndex();
  QModelIndex resultingIndex;
  if( model()->hasIndex(  currentIndex.row() + rowDelta, currentIndex.column(), currentIndex.parent() ) )
    {
      resultingIndex = currentIndex.sibling( currentIndex.row() + rowDelta, currentIndex.column() );
    }
    else //if we scroll into a the parent node...
      {
        QModelIndex parent = currentIndex.parent();
        QModelIndex aunt = parent.sibling( parent.row() + rowDelta, parent.column() );
        if( rowDelta == -1 )
	  resultingIndex = aunt.child( model()->rowCount( aunt ) - 1, 0 );
        else
	  resultingIndex = aunt.child( 0, 0 );
        if( !resultingIndex.isValid() )
	  return;
      }
  selectionModel()->setCurrentIndex( resultingIndex, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows );
  selectionModel()->select( resultingIndex, QItemSelectionModel::ClearAndSelect );

}

QSize BufferView::sizeHint() const {
  return QTreeView::sizeHint();

  if(!model())
    return QTreeView::sizeHint();

  if(model()->rowCount() == 0)
    return QSize(120, 50);

  int columnSize = 0;
  for(int i = 0; i < model()->columnCount(); i++) {
    if(!isColumnHidden(i))
      columnSize += sizeHintForColumn(i);
  }
  return QSize(columnSize, 50);
}

// ==============================
//  BufferView Dock
// ==============================
BufferViewDock::BufferViewDock(BufferViewConfig *config, QWidget *parent)
  : QDockWidget(config->bufferViewName(), parent)
{
  setObjectName("BufferViewDock-" + QString::number(config->bufferViewId()));
  toggleViewAction()->setData(config->bufferViewId());
  setAllowedAreas(Qt::RightDockWidgetArea|Qt::LeftDockWidgetArea);
  connect(config, SIGNAL(bufferViewNameSet(const QString &)), this, SLOT(bufferViewRenamed(const QString &)));
}

void BufferViewDock::bufferViewRenamed(const QString &newName) {
  setWindowTitle(newName);
  toggleViewAction()->setText(newName);
}
