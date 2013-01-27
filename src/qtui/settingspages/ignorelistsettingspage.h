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

#ifndef IGNORELISTSETTINGSPAGE_H
#define IGNORELISTSETTINGSPAGE_H

#include <QStyledItemDelegate>
#include <QButtonGroup>

#include "settingspage.h"
#include "ui_ignorelistsettingspage.h"
#include "ui_ignorelisteditdlg.h"
#include "ignorelistmodel.h"
#include "clientignorelistmanager.h"

class QEvent;
class QPainter;
class QAbstractItemModel;

// the delegate is used to draw the checkbox in column 0
class IgnoreListDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    IgnoreListDelegate(QWidget *parent = 0) : QStyledItemDelegate(parent) {}
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
        const QModelIndex &index) const;
    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option,
        const QModelIndex &index);
};


class IgnoreListEditDlg : public QDialog
{
    Q_OBJECT

public:
    IgnoreListEditDlg(const IgnoreListManager::IgnoreListItem &item, QWidget *parent = 0, bool enabled = false);
    inline IgnoreListManager::IgnoreListItem ignoreListItem() { return _ignoreListItem; }
    void enableOkButton(bool state);

private slots:
    void widgetHasChanged();
    void aboutToAccept() { _ignoreListItem = _clonedIgnoreListItem; }

private:
    IgnoreListManager::IgnoreListItem _ignoreListItem;
    IgnoreListManager::IgnoreListItem _clonedIgnoreListItem;
    bool _hasChanged;
    Ui::IgnoreListEditDlg ui;
    QButtonGroup _typeButtonGroup;
    QButtonGroup _strictnessButtonGroup;
    QButtonGroup _scopeButtonGroup;
};


class IgnoreListSettingsPage : public SettingsPage
{
    Q_OBJECT

public:
    IgnoreListSettingsPage(QWidget *parent = 0);
    ~IgnoreListSettingsPage();
    virtual inline bool hasDefaults() const { return false; }
    virtual inline bool needsCoreConnection() const { return true; }
    void editIgnoreRule(const QString &ignoreRule);

public slots:
    void save();
    void load();
    void defaults();
    void newIgnoreRule(QString rule = QString());

private slots:
    void enableDialog(bool);
    void deleteSelectedIgnoreRule();
    void editSelectedIgnoreRule();
    void selectionChanged(const QItemSelection &selection, const QItemSelection &);

private:
    IgnoreListDelegate *_delegate;
    Ui::IgnoreListSettingsPage ui;
    IgnoreListModel _ignoreListModel;
};


#endif //IGNORELISTSETTINGSPAGE_H
