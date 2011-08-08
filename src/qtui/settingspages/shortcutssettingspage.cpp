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

#include <QTimer>

#include "shortcutssettingspage.h"

#include "action.h"
#include "actioncollection.h"
#include "qtui.h"
#include "shortcutsmodel.h"
#include "util.h"

ShortcutsFilter::ShortcutsFilter(QList<QString> include, QObject *parent) : QSortFilterProxyModel(parent),
  _includeCategories(include)
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
    QModelIndex index;
    if (!source_parent.isValid()) {
        index = sourceModel()->index(source_row, 0);
        Q_ASSERT(index.isValid());
        if(!_includeCategories.contains(qvariant_cast<QString>(index.data())))
            return false;

        return true;
    }

    index = source_parent.model()->index(source_row, 0, source_parent);
    Q_ASSERT(index.isValid());
    if (!qobject_cast<Action *>(index.data(ShortcutsModel::ActionRole).value<QObject *>())->isShortcutConfigurable())
        return false;

    if (!_includeCategories.contains(qvariant_cast<QString>(source_parent.data())))
        return false;

    for (int col = 0; col < source_parent.model()->columnCount(source_parent); col++) {
        if (source_parent.model()->index(source_row, col, source_parent).data().toString().contains(_filterString, Qt::CaseInsensitive))
            return true;
    }
    return false;
}

/****************************************************************************/

ShortcutsSettingsPage::ShortcutsSettingsPage(const QHash<QString, ActionCollection *> &actionCollections, QList<QString> includeCategories, QWidget *parent, const QString &category, const QString &name)
    : SettingsPage(category, name, parent),
    _shortcutsModel(new ShortcutsModel(actionCollections, this)),
    _shortcutsFilter(new ShortcutsFilter(includeCategories, this))
{
    ui.setupUi(this);

    _shortcutsFilter->setSourceModel(_shortcutsModel);
    ui.shortcutsView->setModel(_shortcutsFilter);
    ui.shortcutsView->expandAll();
    ui.shortcutsView->resizeColumnToContents(0);
    ui.shortcutsView->sortByColumn(0, Qt::AscendingOrder);

    ui.keySequenceWidget->setModel(_shortcutsModel);
    connect(ui.keySequenceWidget, SIGNAL(keySequenceChanged(QKeySequence, QModelIndex)), SLOT(keySequenceChanged(QKeySequence, QModelIndex)));

    connect(ui.shortcutsView->selectionModel(), SIGNAL(currentChanged(QModelIndex, QModelIndex)), SLOT(setWidgetStates()));

    setWidgetStates();

    connect(ui.useDefault, SIGNAL(clicked(bool)), SLOT(toggledCustomOrDefault()));
    connect(ui.useCustom, SIGNAL(clicked(bool)), SLOT(toggledCustomOrDefault()));

    connect(_shortcutsModel, SIGNAL(hasChanged(bool)), SLOT(setChangedState(bool)));

    // fugly, but directly setting it from the ctor doesn't seem to work
    QTimer::singleShot(0, ui.searchEdit, SLOT(setFocus()));
}


/*void ShortcutsSettingsPage::initUi()
{

}*/


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
