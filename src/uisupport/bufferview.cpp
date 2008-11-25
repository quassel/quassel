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

#include "uisettings.h"

#include <QAction>
#include <QFlags>
#include <QHeaderView>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QSet>

/*****************************************
* The TreeView showing the Buffers
*****************************************/
// Please be carefull when reimplementing methods which are used to inform the view about changes to the data
// to be on the safe side: call QTreeView's method aswell
BufferView::BufferView(QWidget *parent)
  : QTreeView(parent),
    showChannelList(tr("Show Channel List"), this),
    _menuActions(this)
{
  setupMenuActions();
  showChannelList.setIcon(SmallIcon("format-list-unordered"));

  connect(this, SIGNAL(collapsed(const QModelIndex &)), SLOT(on_collapse(const QModelIndex &)));
  connect(this, SIGNAL(expanded(const QModelIndex &)), SLOT(on_expand(const QModelIndex &)));

  setSelectionMode(QAbstractItemView::ExtendedSelection);
}

void BufferView::init() {
  setIndentation(10);
  header()->setContextMenuPolicy(Qt::ActionsContextMenu);
  hideColumn(1);
  hideColumn(2);
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
#if defined Q_WS_QWS or defined Q_WS_X11
  connect(this, SIGNAL(doubleClicked(QModelIndex)), SLOT(joinChannel(QModelIndex)));
#else
  // afaik this is better on Mac and Windows
  connect(this, SIGNAL(activated(QModelIndex)), SLOT(joinChannel(QModelIndex)));
#endif
}

void BufferView::setupMenuActions() {
  _menuActions.addAction("ConnectNet", new Action(SmallIcon("network-connect"), tr("Connect"), this));
  _menuActions.addAction("DisconnectNet", new Action(SmallIcon("network-disconnect"), tr("Disconnect"), this));
  _menuActions.addAction("JoinChannel", new Action(tr("Join Channel"), this));
  _menuActions.addAction("JoinBuffer", new Action(tr("Join"), this));
  _menuActions.addAction("PartBuffer", new Action(tr("Part"), this));
  _menuActions.addAction("HideBuffersTemporarily", new Action(tr("Hide Buffers"), this));
  _menuActions.addAction("HideBuffersPermanently", new Action(tr("Hide Buffers Permanently"), this));
  _menuActions.addAction("RemoveBuffer", new Action(tr("Delete Buffer"), this));
  _menuActions.addAction("IgnoreList", new Action(tr("Ignore List"), this));
  _menuActions.addAction("HideJoin", new Action(tr("Joins"), this))->setCheckable(true);
  _menuActions.addAction("HidePart", new Action(tr("Parts"), this))->setCheckable(true);
  _menuActions.addAction("HideQuit", new Action(tr("Quits"), this))->setCheckable(true);
  _menuActions.addAction("HideNick", new Action(tr("Nick Changes"), this))->setCheckable(true);
  _menuActions.addAction("HideMode", new Action(tr("Mode Changes"), this))->setCheckable(true);
  _menuActions.addAction("HideDayChange", new Action(tr("Day Change"), this))->setCheckable(true);

  connect(&_menuActions, SIGNAL(actionTriggered(QAction *)), SLOT(actionTriggered(QAction *)));
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
    setRootIndex(QModelIndex());
  }
}

void BufferView::setRootIndexForNetworkId(const NetworkId &networkId) {
  if(!networkId.isValid() || !model()) {
    setRootIndex(QModelIndex());
  } else {
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

bool BufferView::checkRequirements(const QModelIndex &index, ItemActiveStates requiredActiveState) {
  if(!index.isValid())
    return false;

  ItemActiveStates isActive = index.data(NetworkModel::ItemActiveRole).toBool()
    ? ActiveState
    : InactiveState;

  if(!(isActive & requiredActiveState))
    return false;

  return true;
}

void BufferView::addItemToMenu(QAction *action, QMenu *menu, const QModelIndex &index, ItemActiveStates requiredActiveState) {
  addItemToMenu(action, menu, checkRequirements(index, requiredActiveState));
}

void BufferView::addItemToMenu(QAction *action, QMenu *menu, bool condition) {
  if(condition) {
    menu->addAction(action);
    action->setVisible(true);
  } else {
    action->setVisible(false);
  }
}

void BufferView::addItemToMenu(QMenu *subMenu, QMenu *menu, const QModelIndex &index, ItemActiveStates requiredActiveState) {
  if(checkRequirements(index, requiredActiveState)) {
    menu->addMenu(subMenu);
    subMenu->setVisible(true);
  } else {
    subMenu->setVisible(false);
  }
}

void BufferView::addSeparatorToMenu(QMenu *menu, const QModelIndex &index, ItemActiveStates requiredActiveState) {
  if(checkRequirements(index, requiredActiveState)) {
    menu->addSeparator();
  }
}

QMenu *BufferView::createHideEventsSubMenu(QMenu *menu, BufferId bufferId) {
  int filter = BufferSettings(bufferId).messageFilter();
  _menuActions.action("HideJoin")->setChecked(filter & Message::Join);
  _menuActions.action("HidePart")->setChecked(filter & Message::Part);
  _menuActions.action("HideQuit")->setChecked(filter & Message::Quit);
  _menuActions.action("HideNick")->setChecked(filter & Message::Nick);
  _menuActions.action("HideMode")->setChecked(filter & Message::Mode);
  _menuActions.action("HideDayChange")->setChecked(filter & Message::DayChange);

  QMenu *hideEventsMenu = menu->addMenu(tr("Hide Events"));
  hideEventsMenu->addAction(_menuActions.action("HideJoin"));
  hideEventsMenu->addAction(_menuActions.action("HidePart"));
  hideEventsMenu->addAction(_menuActions.action("HideQuit"));
  hideEventsMenu->addAction(_menuActions.action("HideNick"));
  hideEventsMenu->addAction(_menuActions.action("HideMode"));
  hideEventsMenu->addAction(_menuActions.action("HideDayChange"));
  return hideEventsMenu;
}

void BufferView::contextMenuEvent(QContextMenuEvent *event) {
  QModelIndex index = indexAt(event->pos());
  if(!index.isValid())
    index = rootIndex();
  if(!index.isValid())
    return;

  QMenu contextMenu(this);
  addActionsToMenu(&contextMenu, index);
  if(!contextMenu.actions().isEmpty())
    contextMenu.exec(QCursor::pos());

}

void BufferView::addActionsToMenu(QMenu *contextMenu, const QModelIndex &index) {
  _menuIndex = index;
  const Network *network = Client::network(index.data(NetworkModel::NetworkIdRole).value<NetworkId>());
  Q_CHECK_PTR(network);

  NetworkModel::itemType itemType = static_cast<NetworkModel::itemType>(index.data(NetworkModel::ItemTypeRole).toInt());

  switch(itemType) {
  case NetworkModel::NetworkItemType:
    showChannelList.setData(index.data(NetworkModel::NetworkIdRole));
    addItemToMenu(&showChannelList, contextMenu, index, ActiveState);
    addItemToMenu(_menuActions.action("DisconnectNet"), contextMenu, network->connectionState() != Network::Disconnected);
    addItemToMenu(_menuActions.action("ConnectNet"), contextMenu, network->connectionState() == Network::Disconnected);
    addSeparatorToMenu(contextMenu, index, ActiveState);
    addItemToMenu(_menuActions.action("JoinChannel"), contextMenu, index, ActiveState);
    break;
  case NetworkModel::BufferItemType:
    {
      BufferInfo bufferInfo = index.data(NetworkModel::BufferInfoRole).value<BufferInfo>();
      switch(bufferInfo.type()) {
      case BufferInfo::ChannelBuffer:
        addItemToMenu(_menuActions.action("JoinBuffer"), contextMenu, index, InactiveState);
        addItemToMenu(_menuActions.action("PartBuffer"), contextMenu, index, ActiveState);
        addItemToMenu(_menuActions.action("HideBuffersTemporarily"), contextMenu, (bool)config());
        addItemToMenu(_menuActions.action("HideBuffersPermanently"), contextMenu, (bool)config());
        addItemToMenu(_menuActions.action("RemoveBuffer"), contextMenu, index, InactiveState);
        createHideEventsSubMenu(contextMenu, bufferInfo.bufferId());
        addItemToMenu(_menuActions.action("IgnoreList"), contextMenu);
        break;
      case BufferInfo::QueryBuffer:
        addItemToMenu(_menuActions.action("HideBuffersTemporarily"), contextMenu, (bool)config());
        addItemToMenu(_menuActions.action("HideBuffersPermanently"), contextMenu, (bool)config());
        addItemToMenu(_menuActions.action("RemoveBuffer"), contextMenu);
        createHideEventsSubMenu(contextMenu, bufferInfo.bufferId());
        break;
      default:
        addItemToMenu(_menuActions.action("HideBuffersTemporarily"), contextMenu, (bool)config());
        addItemToMenu(_menuActions.action("HideBuffersPermanently"), contextMenu, (bool)config());
        break;
      }
    }
    break;
  default:
    return;
  }
}

void BufferView::actionTriggered(QAction *result) {
  const Network *network = Client::network(_menuIndex.data(NetworkModel::NetworkIdRole).value<NetworkId>());
  Q_CHECK_PTR(network);

  if(network && result == _menuActions.action("ConnectNet")) {
    network->requestConnect();
    return;
  }

  if(network && result == _menuActions.action("DisconnectNet")) {
    network->requestDisconnect();
    return;
  }

if(result == _menuActions.action("JoinChannel")) {
    // FIXME no QInputDialog in Qtopia
#ifndef Q_WS_QWS
    bool ok;
    QString channelName = QInputDialog::getText(this, tr("Join Channel"), tr("Input channel name:"), QLineEdit::Normal, QString(), &ok);
    if(ok && !channelName.isEmpty()) {
      Client::instance()->userInput(BufferInfo::fakeStatusBuffer(_menuIndex.data(NetworkModel::NetworkIdRole).value<NetworkId>()),
                                     QString("/J %1").arg(channelName));
    }
#endif
    return;
  }

  if(result == _menuActions.action("JoinBuffer")) {
    BufferInfo bufferInfo = _menuIndex.data(NetworkModel::BufferInfoRole).value<BufferInfo>();
    Client::instance()->userInput(bufferInfo, QString("/JOIN %1").arg(bufferInfo.bufferName()));
    return;
  }

  if(result == _menuActions.action("PartBuffer")) {
    BufferInfo bufferInfo = _menuIndex.data(NetworkModel::BufferInfoRole).value<BufferInfo>();
    Client::instance()->userInput(bufferInfo, QString("/PART"));
    return;
  }

  if(result == _menuActions.action("HideBuffersTemporarily")) {
    removeSelectedBuffers();
    return;
  }

  if(result == _menuActions.action("HideBuffersPermanently")) {
    removeSelectedBuffers(true);
    return;
  }

  if(result == _menuActions.action("RemoveBuffer")) {
    BufferInfo bufferInfo = _menuIndex.data(NetworkModel::BufferInfoRole).value<BufferInfo>();
    int res = QMessageBox::question(this, tr("Remove buffer permanently?"),
                                    tr("Do you want to delete the buffer \"%1\" permanently? This will delete all related data, including all backlog "
                                       "data, from the core's database!").arg(bufferInfo.bufferName()),
                                        QMessageBox::Yes|QMessageBox::No, QMessageBox::No);
    if(res == QMessageBox::Yes) {
      Client::removeBuffer(bufferInfo.bufferId());
    }
    return;
  }

  if(result == _menuActions.action("HideJoin")) {
    BufferId bufferId = _menuIndex.data(NetworkModel::BufferIdRole).value<BufferId>();
    BufferSettings(bufferId).filterMessage(Message::Join, result->isChecked());
    return;
  }
  if(result == _menuActions.action("HidePart")) {
    BufferId bufferId = _menuIndex.data(NetworkModel::BufferIdRole).value<BufferId>();
    BufferSettings(bufferId).filterMessage(Message::Part, result->isChecked());
    return;
  }
  if(result == _menuActions.action("HideQuit")) {
    BufferId bufferId = _menuIndex.data(NetworkModel::BufferIdRole).value<BufferId>();
    BufferSettings(bufferId).filterMessage(Message::Quit, result->isChecked());
    return;
  }
  if(result == _menuActions.action("HideNick")) {
    BufferId bufferId = _menuIndex.data(NetworkModel::BufferIdRole).value<BufferId>();
    BufferSettings(bufferId).filterMessage(Message::Nick, result->isChecked());
    return;
  }
  if(result == _menuActions.action("HideMode")) {
    BufferId bufferId = _menuIndex.data(NetworkModel::BufferIdRole).value<BufferId>();
    BufferSettings(bufferId).filterMessage(Message::Mode, result->isChecked());
    return;
  }
  if(result == _menuActions.action("HideDayChange")) {
    BufferId bufferId = _menuIndex.data(NetworkModel::BufferIdRole).value<BufferId>();
    BufferSettings(bufferId).filterMessage(Message::DayChange, result->isChecked());
    return;
  }

  qWarning() << "Unhandled menu action";
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

BufferViewDock::BufferViewDock(QWidget *parent)
  : QDockWidget(tr("All Buffers"), parent)
{
  setObjectName("BufferViewDock--1");
  toggleViewAction()->setData((int)-1);
  setAllowedAreas(Qt::RightDockWidgetArea|Qt::LeftDockWidgetArea);
}

void BufferViewDock::bufferViewRenamed(const QString &newName) {
  setWindowTitle(newName);
  toggleViewAction()->setText(newName);
}
