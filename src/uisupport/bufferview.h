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

#ifndef BUFFERVIEW_H_
#define BUFFERVIEW_H_

#include <QAction>
#include <QMenu>
#include <QDockWidget>
#include <QModelIndex>
#include <QTreeView>
#include <QPointer>

#include "bufferviewconfig.h"
#include "networkmodel.h"

#include "types.h"

/*****************************************
 * The TreeView showing the Buffers
 *****************************************/
class BufferView : public QTreeView {
  Q_OBJECT

public:
  BufferView(QWidget *parent = 0);
  void init();
  
  void setModel(QAbstractItemModel *model);
  void setFilteredModel(QAbstractItemModel *model, BufferViewConfig *config);
  virtual void setSelectionModel(QItemSelectionModel *selectionModel);
  
  void setConfig(BufferViewConfig *config);
  inline BufferViewConfig *config() { return _config; }

public slots:
  void setRootIndexForNetworkId(const NetworkId &networkId);
  void removeSelectedBuffers(bool permanently = false);
  
signals:
  void removeBuffer(const QModelIndex &);
  void removeBufferPermanently(const QModelIndex &);
  
protected:
  virtual void keyPressEvent(QKeyEvent *);
  virtual void rowsInserted(const QModelIndex & parent, int start, int end);
  virtual void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
  virtual void wheelEvent(QWheelEvent *);
  virtual QSize sizeHint() const;
  virtual void focusInEvent(QFocusEvent *event) { QAbstractScrollArea::focusInEvent(event); }
  virtual void contextMenuEvent(QContextMenuEvent *event);

private slots:
  void joinChannel(const QModelIndex &index);
  void toggleHeader(bool checked);

  void on_collapse(const QModelIndex &index);
  void on_expand(const QModelIndex &index);
  void on_configChanged();

private:
  enum ItemActiveState {
    InactiveState = 0x01,
    ActiveState = 0x02
  };

public:
  Q_DECLARE_FLAGS(ItemActiveStates, ItemActiveState)
  QAction showChannelList;

private:
  QPointer<BufferViewConfig> _config;
  
  QAction _connectNetAction;
  QAction _disconnectNetAction;
  QAction _joinChannelAction;
  
  QAction _joinBufferAction;
  QAction _partBufferAction;
  QAction _hideBufferTemporarilyAction;
  QAction _hideBufferPermanentlyAction;
  QAction _removeBufferAction;
  QAction _ignoreListAction;
  
  QAction _hideJoinAction;
  QAction _hidePartAction;
  QAction _hideQuitAction;
  QAction _hideModeAction;

  QHash<NetworkId, bool> _expandedState;

  void storeExpandedState(NetworkId networkId, bool expanded);

  bool checkRequirements(const QModelIndex &index,
			 ItemActiveStates requiredActiveState = QFlags<ItemActiveState>(ActiveState) | QFlags<ItemActiveState>(InactiveState));
  void addItemToMenu(QAction &action, QMenu &menu, const QModelIndex &index,
		     ItemActiveStates requiredActiveState = QFlags<ItemActiveState>(ActiveState) | QFlags<ItemActiveState>(InactiveState));
  void addItemToMenu(QAction &action, QMenu &menu, bool condition = true);
  void addItemToMenu(QMenu &subMenu, QMenu &menu, const QModelIndex &index,
		     ItemActiveStates requiredActiveState = QFlags<ItemActiveState>(ActiveState) | QFlags<ItemActiveState>(InactiveState));
  void addSeparatorToMenu(QMenu &menu, const QModelIndex &index,
			  ItemActiveStates requiredActiveState = QFlags<ItemActiveState>(ActiveState) | QFlags<ItemActiveState>(InactiveState));
  QMenu *createHideEventsSubMenu(QMenu &menu, BufferId bufferId);
};
Q_DECLARE_OPERATORS_FOR_FLAGS(BufferView::ItemActiveStates)


// ==============================
//  BufferView Dock
// ==============================
class BufferViewDock : public QDockWidget {
  Q_OBJECT

public:
  BufferViewDock(BufferViewConfig *config, QWidget *parent);
  BufferViewDock(QWidget *parent);
				 
public slots:
  void bufferViewRenamed(const QString &newName);
};

#endif

