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

#include "nicklistwidget.h"

#include "buffer.h"
#include "nickview.h"
#include "client.h"
#include "networkmodel.h"
#include "buffermodel.h"
#include "nickviewfilter.h"
#include "qtuisettings.h"

NickListWidget::NickListWidget(QWidget *parent)
  : AbstractItemView(parent),
    _showNickListAction(new QAction(tr("Nicks"), this)),
    _showDockAction(0)
{
  _showNickListAction->setCheckable(true);
  QtUiSettings s;
  _showNickListAction->setChecked(s.value("ShowNickListAction", QVariant(true)).toBool());
  ui.setupUi(this);
  connect(_showNickListAction, SIGNAL(toggled(bool)), this, SLOT(showWidget(bool)));
}

NickListWidget::~NickListWidget() {
  QtUiSettings s;
  s.setValue("ShowNickListAction", showNickListAction()->isChecked());
}

void NickListWidget::setShowDockAction(QAction *action) {
  _showDockAction = action;
}

QAction *NickListWidget::showDockAction() const {
  return _showDockAction;
}

QAction *NickListWidget::showNickListAction() const {
  return _showNickListAction;
}

void NickListWidget::showWidget(bool visible) {
  if(!selectionModel())
    return;

  QModelIndex currentIndex = selectionModel()->currentIndex();
  if(currentIndex.data(NetworkModel::BufferTypeRole) == BufferInfo::ChannelBuffer && showDockAction()) {
    if(visible  != showDockAction()->isChecked()) {
      // show or hide
      showDockAction()->trigger();
    }
  }
}

void NickListWidget::changedVisibility(bool visible) {
  if(!selectionModel())
    return;

  QModelIndex currentIndex = selectionModel()->currentIndex();
  if(currentIndex.data(NetworkModel::BufferTypeRole) == BufferInfo::ChannelBuffer && !visible) {
    showNickListAction()->setChecked(false);
  }
}

void NickListWidget::currentChanged(const QModelIndex &current, const QModelIndex &previous) {
  BufferInfo::Type bufferType = (BufferInfo::Type)current.data(NetworkModel::BufferTypeRole).toInt();
  BufferId newBufferId = current.data(NetworkModel::BufferIdRole).value<BufferId>();
  BufferId oldBufferId = previous.data(NetworkModel::BufferIdRole).value<BufferId>();

  if(bufferType != BufferInfo::ChannelBuffer) {
    ui.stackedWidget->setCurrentWidget(ui.emptyPage);
    if(showDockAction() && showDockAction()->isChecked()) {
      // hide
      showDockAction()->trigger();
    }
    return;
  } else {
    if(showNickListAction()->isChecked())
      if(showDockAction()&& !showDockAction()->isChecked()) {
        // show
        showDockAction()->trigger();
      }
  }

  if(newBufferId == oldBufferId)
    return;

  if(nickViews.contains(newBufferId)) {
    ui.stackedWidget->setCurrentWidget(nickViews.value(newBufferId));
  } else {
    NickView *view = new NickView(this);
    NickViewFilter *filter = new NickViewFilter(newBufferId, Client::networkModel());
    view->setModel(filter);
    QModelIndex source_current = Client::bufferModel()->mapToSource(current);
    view->setRootIndex(filter->mapFromSource(source_current));
    view->expandAll();
    nickViews[newBufferId] = view;
    ui.stackedWidget->addWidget(view);
    ui.stackedWidget->setCurrentWidget(view);
  }
}

void NickListWidget::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end) {
  Q_ASSERT(model());
  if(!parent.isValid()) {
    // ok this means that whole networks are about to be removed
    // we can't determine which buffers are affect, so we hope that all nets are removed
    // this is the most common case (for example disconnecting from the core or terminating the clint)
    NickView *nickView;
    QHash<BufferId, NickView *>::iterator iter = nickViews.begin();
    while(iter != nickViews.end()) {
      nickView = *iter;
      iter = nickViews.erase(iter);
      ui.stackedWidget->removeWidget(nickView);
      QAbstractItemModel *model = nickView->model();
      nickView->setModel(0);
      if(QSortFilterProxyModel *filter = qobject_cast<QSortFilterProxyModel *>(model))
	filter->setSourceModel(0);
      model->deleteLater();
      nickView->deleteLater();
    }
  } else {
    // check if there are explicitly buffers removed
    for(int i = start; i <= end; i++) {
      QVariant variant = parent.child(i,0).data(NetworkModel::BufferIdRole);
      if(!variant.isValid())
	continue;

      BufferId bufferId = qVariantValue<BufferId>(variant);
      removeBuffer(bufferId);
    }
  }
}

void NickListWidget::removeBuffer(BufferId bufferId) {
  if(!nickViews.contains(bufferId))
    return;

  NickView *view = nickViews.take(bufferId);
  ui.stackedWidget->removeWidget(view);
  QAbstractItemModel *model = view->model();
  view->setModel(0);
  if(QSortFilterProxyModel *filter = qobject_cast<QSortFilterProxyModel *>(model))
    filter->setSourceModel(0);
  model->deleteLater();
  view->deleteLater();
}

QSize NickListWidget::sizeHint() const {
  QWidget *currentWidget = ui.stackedWidget->currentWidget();
  if(!currentWidget || currentWidget == ui.emptyPage)
    return QSize(100, height());
  else
    return currentWidget->sizeHint();
}
