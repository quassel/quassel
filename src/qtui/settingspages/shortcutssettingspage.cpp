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

#include <QTimer>

#include "shortcutssettingspage.h"

#include "action.h"
#include "actioncollection.h"
#include "qtui.h"
#include "shortcutsmodel.h"
#include "util.h"

ShortcutsFilter::ShortcutsFilter(QObject *parent) : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
}


void ShortcutsFilter::setFilterString(const QString &filterString)
{
    _filterString = filterString;
    invalidateFilter();
}


bool ShortcutsFilter::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    if (!source_parent.isValid())
        return true;

    QModelIndex index = source_parent.model()->index(source_row, 0, source_parent);
    Q_ASSERT(index.isValid());
    if (!qobject_cast<Action *>(index.data(ShortcutsModel::ActionRole).value<QObject *>())->isShortcutConfigurable())
        return false;

    for (int col = 0; col < source_parent.model()->columnCount(source_parent); col++) {
        if (source_parent.model()->index(source_row, col, source_parent).data().toString().contains(_filterString, Qt::CaseInsensitive))
            return true;
    }
    return false;
}


/****************************************************************************/

ShortcutsSettingsPage::ShortcutsSettingsPage(const QHash<QString, ActionCollection *> &actionCollections, QWidget *parent)
    : SettingsPage(tr("Interface"), tr("Shortcuts"), parent),
    _shortcutsModel(new ShortcutsModel(actionCollections, this)),
    _shortcutsFilter(new ShortcutsFilter(this))
{
    ui.setupUi(this);

    _shortcutsFilter->setSourceModel(_shortcutsModel);
    ui.shortcutsView->setModel(_shortcutsFilter);
    ui.shortcutsView->expandAll();
    ui.shortcutsView->resizeColumnToContents(0);
    ui.shortcutsView->sortByColumn(0, Qt::AscendingOrder);

    ui.keySequenceWidget->setModel(_shortcutsModel);
    connect(ui.keySequenceWidget, &KeySequenceWidget::keySequenceChanged, this, &ShortcutsSettingsPage::keySequenceChanged);

    connect(ui.shortcutsView->selectionModel(), &QItemSelectionModel::currentChanged, this, &ShortcutsSettingsPage::setWidgetStates);

    setWidgetStates();

    connect(ui.useDefault, &QAbstractButton::clicked, this, &ShortcutsSettingsPage::toggledCustomOrDefault);
    connect(ui.useCustom, &QAbstractButton::clicked, this, &ShortcutsSettingsPage::toggledCustomOrDefault);

    connect(_shortcutsModel, SIGNAL(hasChanged(bool)), SLOT(setChangedState(bool)));

    // fugly, but directly setting it from the ctor doesn't seem to work
    QTimer::singleShot(0, ui.searchEdit, SLOT(setFocus()));
}


void ShortcutsSettingsPage::setWidgetStates()
{
    if (ui.shortcutsView->currentIndex().isValid() && ui.shortcutsView->currentIndex().parent().isValid()) {
        QKeySequence active = ui.shortcutsView->currentIndex().data(ShortcutsModel::ActiveShortcutRole).value<QKeySequence>();
        QKeySequence def = ui.shortcutsView->currentIndex().data(ShortcutsModel::DefaultShortcutRole).value<QKeySequence>();
        ui.defaultShortcut->setText(def.isEmpty() ? tr("None") : def.toString(QKeySequence::NativeText));
        ui.actionBox->setEnabled(true);
        if (active == def) {
            ui.useDefault->setChecked(true);
            ui.keySequenceWidget->setKeySequence(QKeySequence());
        }
        else {
            ui.useCustom->setChecked(true);
            ui.keySequenceWidget->setKeySequence(active);
        }
    }
    else {
        ui.defaultShortcut->setText(tr("None"));
        ui.actionBox->setEnabled(false);
        ui.useDefault->setChecked(true);
        ui.keySequenceWidget->setKeySequence(QKeySequence());
    }
}


void ShortcutsSettingsPage::on_searchEdit_textChanged(const QString &text)
{
    _shortcutsFilter->setFilterString(text);
}


void ShortcutsSettingsPage::keySequenceChanged(const QKeySequence &seq, const QModelIndex &conflicting)
{
    if (conflicting.isValid())
        _shortcutsModel->setData(conflicting, QKeySequence(), ShortcutsModel::ActiveShortcutRole);

    QModelIndex rowIdx = _shortcutsFilter->mapToSource(ui.shortcutsView->currentIndex());
    Q_ASSERT(rowIdx.isValid());
    _shortcutsModel->setData(rowIdx, seq, ShortcutsModel::ActiveShortcutRole);
    setWidgetStates();
}


void ShortcutsSettingsPage::toggledCustomOrDefault()
{
    if (!ui.shortcutsView->currentIndex().isValid())
        return;

    QModelIndex index = _shortcutsFilter->mapToSource(ui.shortcutsView->currentIndex());
    Q_ASSERT(index.isValid());

    if (ui.useDefault->isChecked()) {
        _shortcutsModel->setData(index, index.data(ShortcutsModel::DefaultShortcutRole));
    }
    else {
        _shortcutsModel->setData(index, QKeySequence());
    }
    setWidgetStates();
}


void ShortcutsSettingsPage::save()
{
    _shortcutsModel->commit();
    QtUi::saveShortcuts();
    SettingsPage::save();
}


void ShortcutsSettingsPage::load()
{
    _shortcutsModel->load();

    SettingsPage::load();
}


void ShortcutsSettingsPage::defaults()
{
    _shortcutsModel->defaults();

    SettingsPage::defaults();
}
