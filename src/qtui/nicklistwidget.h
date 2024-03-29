/***************************************************************************
 *   Copyright (C) 2005-2022 by the Quassel Project                        *
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

#include <QDockWidget>
#include <QHash>

#include "abstractitemview.h"
#include "buffermodel.h"
#include "types.h"

#include "ui_nicklistwidget.h"

class Buffer;
class NickView;
class QDockWidget;

class NickListWidget : public AbstractItemView
{
    Q_OBJECT

public:
    NickListWidget(QWidget* parent = nullptr);

public slots:
    void showWidget(bool visible);
    void setVisible(bool visible) override;

signals:
    void nickSelectionChanged(const QModelIndexList&);

protected:
    QSize sizeHint() const override;
    void hideEvent(QHideEvent*) override;
    void showEvent(QShowEvent*) override;

protected slots:
    void currentChanged(const QModelIndex& current, const QModelIndex& previous) override;
    void rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end) override;

private:
    void removeBuffer(BufferId bufferId);
    void onNickSelectionChanged();

private:
    Ui::NickListWidget ui;
    QHash<BufferId, NickView*> nickViews;

    QDockWidget* dock() const;
};

// ==============================
//  NickList Dock
// ==============================
class NickListDock : public QDockWidget
{
    Q_OBJECT

public:
    NickListDock(const QString& title, QWidget* parent = nullptr);
    // ~NickListDock();

    void setLocked(bool locked);

    // virtual bool event(QEvent *event);
};

#endif
