/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
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

#include "actioncollection.h"
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

  void addActionsToMenu(QMenu *menu, const QModelIndex &index);
  void addFilterActions(QMenu *contextMenu, const QModelIndex &index);

public slots:
  void setRootIndexForNetworkId(const NetworkId &networkId);
  void removeSelectedBuffers(bool permanently = false);
  void menuActionTriggered(QAction *);

signals:
  void removeBuffer(const QModelIndex &);
  void removeBufferPermanently(const QModelIndex &);

protected:
  virtual void keyPressEvent(QKeyEvent *);
  virtual void dropEvent(QDropEvent *event);
  virtual void rowsInserted(const QModelIndex & parent, int start, int end);
  virtual void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
  virtual void wheelEvent(QWheelEvent *);
  virtual QSize sizeHint() const;
  virtual void focusInEvent(QFocusEvent *event) { QAbstractScrollArea::focusInEvent(event); }
  virtual void contextMenuEvent(QContextMenuEvent *event);

private slots:
  void joinChannel(const QModelIndex &index);
  void toggleHeader(bool checked);

  void storeExpandedState(const QModelIndex &networkIdx);
  void setExpandedState(const QModelIndex &networkIdx);

  void on_configChanged();

private:
  QPointer<BufferViewConfig> _config;

  enum ExpandedState {
    WasExpanded = 0x01,
    WasActive = 0x02
  };
  QHash<NetworkId, short> _expandedState;

};

// ******************************
//  TristateDelgate
// ******************************
#include <QStyledItemDelegate>

class TristateDelegate : public QStyledItemDelegate {
  Q_OBJECT

public:
  TristateDelegate(QObject *parent = 0) : QStyledItemDelegate(parent) {}
  bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index);
};


// ==============================
//  BufferView Dock
// ==============================
class BufferViewDock : public QDockWidget {
  Q_OBJECT

public:
  BufferViewDock(BufferViewConfig *config, QWidget *parent);

  inline BufferView *bufferView() const { return qobject_cast<BufferView *>(widget()); }

public slots:
  void bufferViewRenamed(const QString &newName);
};

#endif
