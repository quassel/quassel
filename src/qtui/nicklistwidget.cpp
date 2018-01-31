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

#include "nicklistwidget.h"

#include "nickview.h"
#include "client.h"
#include "networkmodel.h"
#include "buffermodel.h"
#include "nickviewfilter.h"
#include "qtuisettings.h"

#include <QAction>
#include <QDebug>
#include <QEvent>
#include <QAbstractButton>

NickListWidget::NickListWidget(QWidget *parent)
    : AbstractItemView(parent)
{
    ui.setupUi(this);
}


QDockWidget *NickListWidget::dock() const
{
    QDockWidget *dock = qobject_cast<QDockWidget *>(parent());
    if (dock)
        return dock;
    else
        return 0;
}


void NickListWidget::hideEvent(QHideEvent *event)
{
    emit nickSelectionChanged(QModelIndexList());
    AbstractItemView::hideEvent(event);
}


void NickListWidget::showEvent(QShowEvent *event)
{
    NickView *view = qobject_cast<NickView *>(ui.stackedWidget->currentWidget());
    if (view)
        emit nickSelectionChanged(view->selectedIndexes());

    AbstractItemView::showEvent(event);
}


void NickListWidget::showWidget(bool visible)
{
    if (!selectionModel())
        return;

    QModelIndex currentIndex = selectionModel()->currentIndex();
    if (currentIndex.data(NetworkModel::BufferTypeRole) == BufferInfo::ChannelBuffer) {
        QDockWidget *dock_ = dock();
        if (!dock_)
            return;

        if (visible)
            dock_->show();
        else
            dock_->close();
    }
}

void NickListWidget::setVisible(bool visible)
{
    QWidget::setVisible(visible);
    QDockWidget *dock_ = dock();
    if (!dock_)
        return;

    if (visible)
        dock_->show();
    else
        dock_->close();
}


void NickListWidget::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    BufferInfo::Type bufferType = (BufferInfo::Type)current.data(NetworkModel::BufferTypeRole).toInt();
    BufferId newBufferId = current.data(NetworkModel::BufferIdRole).value<BufferId>();
    BufferId oldBufferId = previous.data(NetworkModel::BufferIdRole).value<BufferId>();

    if (bufferType != BufferInfo::ChannelBuffer) {
        ui.stackedWidget->setCurrentWidget(ui.emptyPage);
        emit nickSelectionChanged(QModelIndexList());
        return;
    }

    // See NickListDock::NickListDock() below
//   if(bufferType != BufferInfo::ChannelBuffer) {
//     ui.stackedWidget->setCurrentWidget(ui.emptyPage);
//     QDockWidget *dock_ = dock();
//     if(dock_) {
//       dock_->close();
//     }
//     return;
//   } else {
//     QDockWidget *dock_ = dock();
//     if(dock_ && dock_->toggleViewAction()->isChecked()) {
//       dock_->show();
//     }
//   }

    if (newBufferId == oldBufferId)
        return;

    NickView *view;
    if (nickViews.contains(newBufferId)) {
        view = nickViews.value(newBufferId);
        ui.stackedWidget->setCurrentWidget(view);
    }
    else {
        view = new NickView(this);
        NickViewFilter *filter = new NickViewFilter(newBufferId, Client::networkModel());
        view->setModel(filter);
        QModelIndex source_current = Client::bufferModel()->mapToSource(current);
        view->setRootIndex(filter->mapFromSource(source_current));
        nickViews[newBufferId] = view;
        ui.stackedWidget->addWidget(view);
        ui.stackedWidget->setCurrentWidget(view);
        connect(view, SIGNAL(selectionUpdated()), SLOT(nickSelectionChanged()));
    }
    emit nickSelectionChanged(view->selectedIndexes());
}


void NickListWidget::nickSelectionChanged()
{
    NickView *view = qobject_cast<NickView *>(sender());
    Q_ASSERT(view);
    if (view != ui.stackedWidget->currentWidget()) {
        qDebug() << "Nick selection of hidden view changed!";
        return;
    }
    emit nickSelectionChanged(view->selectedIndexes());
}


void NickListWidget::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    Q_ASSERT(model());
    if (!parent.isValid()) {
        // ok this means that whole networks are about to be removed
        // we can't determine which buffers are affect, so we hope that all nets are removed
        // this is the most common case (for example disconnecting from the core or terminating the clint)
        NickView *nickView;
        QHash<BufferId, NickView *>::iterator iter = nickViews.begin();
        while (iter != nickViews.end()) {
            nickView = *iter;
            iter = nickViews.erase(iter);
            ui.stackedWidget->removeWidget(nickView);
            QAbstractItemModel *model = nickView->model();
            nickView->setModel(0);
            if (QSortFilterProxyModel *filter = qobject_cast<QSortFilterProxyModel *>(model))
                filter->setSourceModel(0);
            model->deleteLater();
            nickView->deleteLater();
        }
    }
    else {
        // check if there are explicitly buffers removed
        for (int i = start; i <= end; i++) {
            QVariant variant = parent.child(i, 0).data(NetworkModel::BufferIdRole);
            if (!variant.isValid())
                continue;

            BufferId bufferId = variant.value<BufferId>();
            removeBuffer(bufferId);
        }
    }
}


void NickListWidget::removeBuffer(BufferId bufferId)
{
    if (!nickViews.contains(bufferId))
        return;

    NickView *view = nickViews.take(bufferId);
    ui.stackedWidget->removeWidget(view);
    QAbstractItemModel *model = view->model();
    view->setModel(0);
    if (QSortFilterProxyModel *filter = qobject_cast<QSortFilterProxyModel *>(model))
        filter->setSourceModel(0);
    model->deleteLater();
    view->deleteLater();
}


QSize NickListWidget::sizeHint() const
{
    QWidget *currentWidget = ui.stackedWidget->currentWidget();
    if (!currentWidget || currentWidget == ui.emptyPage)
        return QSize(100, height());
    else
        return currentWidget->sizeHint();
}


// ==============================
//  NickList Dock
// ==============================
NickListDock::NickListDock(const QString &title, QWidget *parent)
    : QDockWidget(title, parent)
{
    // THIS STUFF IS NEEDED FOR NICKLIST AUTOHIDE...
    // AS THIS BRINGS LOTS OF FUCKUPS WITH IT IT'S DEACTIVATED FOR NOW...

//   QAction *toggleView = toggleViewAction();
//   disconnect(toggleView, SIGNAL(triggered(bool)), this, 0);
//   toggleView->setChecked(QtUiSettings().value("ShowNickList", QVariant(true)).toBool());

//   // reconnecting the closebuttons clicked signal to the action
//   foreach(QAbstractButton *button, findChildren<QAbstractButton *>()) {
//     if(disconnect(button, SIGNAL(clicked()), this, SLOT(close())))
//       connect(button, SIGNAL(clicked()), toggleView, SLOT(trigger()));
//   }
}

void NickListDock::setLocked(bool locked) {
    if (locked) {
        setFeatures(0);
    }
    else {
        setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    }
}

// NickListDock::~NickListDock() {
//   QtUiSettings().setValue("ShowNickList", toggleViewAction()->isChecked());
// }

// bool NickListDock::event(QEvent *event) {
//   switch (event->type()) {
//   case QEvent::Hide:
//   case QEvent::Show:
//     emit visibilityChanged(event->type() == QEvent::Show);
//     return QWidget::event(event);
//   default:
//     return QDockWidget::event(event);
//   }
// }
