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

#ifndef NICKLISTWIDGET_H_
#define NICKLISTWIDGET_H_

#include "ui_nicklistwidget.h"
#include "abstractitemview.h"
#include "buffermodel.h"

#include "types.h"

#include <QHash>
#include <QDockWidget>

class Buffer;
class NickView;
class QDockWidget;

class NickListWidget : public AbstractItemView
{
    Q_OBJECT

public:
    NickListWidget(QWidget *parent = 0);

public slots:
    void showWidget(bool visible);

signals:
    void nickSelectionChanged(const QModelIndexList &);

protected:
    virtual QSize sizeHint() const;
    virtual void hideEvent(QHideEvent *);
    virtual void showEvent(QShowEvent *);

protected slots:
    virtual void currentChanged(const QModelIndex &current, const QModelIndex &previous);
    virtual void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);

private slots:
    void removeBuffer(BufferId bufferId);
    void nickSelectionChanged();

private:
    Ui::NickListWidget ui;
    QHash<BufferId, NickView *> nickViews;

    QDockWidget *dock() const;
};


// ==============================
//  NickList Dock
// ==============================
class NickListDock : public QDockWidget
{
    Q_OBJECT

public:
    NickListDock(const QString &title, QWidget *parent = 0);
    // ~NickListDock();

    // virtual bool event(QEvent *event);
};


#endif
