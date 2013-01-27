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

#include "ignorelistsettingspage.h"

#include <QHeaderView>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QPainter>
#include <QMessageBox>
#include <QString>
#include <QEvent>
#include <QDebug>
#include "iconloader.h"

IgnoreListSettingsPage::IgnoreListSettingsPage(QWidget *parent)
    : SettingsPage(tr("IRC"), tr("Ignore List"), parent)
{
    ui.setupUi(this);
    _delegate = new IgnoreListDelegate(ui.ignoreListView);
    ui.newIgnoreRuleButton->setIcon(SmallIcon("list-add"));
    ui.deleteIgnoreRuleButton->setIcon(SmallIcon("edit-delete"));
    ui.editIgnoreRuleButton->setIcon(SmallIcon("configure"));

    ui.ignoreListView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui.ignoreListView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui.ignoreListView->setAlternatingRowColors(true);
    ui.ignoreListView->setTabKeyNavigation(false);
    ui.ignoreListView->setModel(&_ignoreListModel);
    // ui.ignoreListView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // ui.ignoreListView->setSortingEnabled(true);
    ui.ignoreListView->verticalHeader()->hide();
    ui.ignoreListView->hideColumn(1);
    ui.ignoreListView->resizeColumnToContents(0);
    ui.ignoreListView->horizontalHeader()->setStretchLastSection(true);
    ui.ignoreListView->setItemDelegateForColumn(0, _delegate);
    ui.ignoreListView->viewport()->setAttribute(Qt::WA_Hover);
    ui.ignoreListView->viewport()->setMouseTracking(true);

    connect(ui.ignoreListView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)), this, SLOT(selectionChanged(const QItemSelection &, const QItemSelection &)));
    connect(ui.newIgnoreRuleButton, SIGNAL(clicked()), this, SLOT(newIgnoreRule()));
    connect(ui.deleteIgnoreRuleButton, SIGNAL(clicked()), this, SLOT(deleteSelectedIgnoreRule()));
    connect(ui.editIgnoreRuleButton, SIGNAL(clicked()), this, SLOT(editSelectedIgnoreRule()));
    connect(&_ignoreListModel, SIGNAL(configChanged(bool)), this, SLOT(setChangedState(bool)));
    connect(&_ignoreListModel, SIGNAL(modelReady(bool)), this, SLOT(enableDialog(bool)));

    enableDialog(_ignoreListModel.isReady());
}


IgnoreListSettingsPage::~IgnoreListSettingsPage()
{
    delete _delegate;
}


void IgnoreListSettingsPage::load()
{
    if (_ignoreListModel.configChanged())
        _ignoreListModel.revert();
    ui.ignoreListView->selectionModel()->reset();
    ui.editIgnoreRuleButton->setEnabled(false);
}


void IgnoreListSettingsPage::defaults()
{
    _ignoreListModel.loadDefaults();
}


void IgnoreListSettingsPage::save()
{
    if (_ignoreListModel.configChanged()) {
        _ignoreListModel.commit();
    }
    ui.ignoreListView->selectionModel()->reset();
    ui.editIgnoreRuleButton->setEnabled(false);
}


void IgnoreListSettingsPage::enableDialog(bool enabled)
{
    ui.newIgnoreRuleButton->setEnabled(enabled);
    setEnabled(enabled);
}


void IgnoreListSettingsPage::selectionChanged(const QItemSelection &selection, const QItemSelection &)
{
    bool state = !selection.isEmpty();
    ui.deleteIgnoreRuleButton->setEnabled(state);
    ui.editIgnoreRuleButton->setEnabled(state);
}


void IgnoreListSettingsPage::deleteSelectedIgnoreRule()
{
    if (!ui.ignoreListView->selectionModel()->hasSelection())
        return;

    _ignoreListModel.removeIgnoreRule(ui.ignoreListView->selectionModel()->selectedIndexes()[0].row());
}


void IgnoreListSettingsPage::newIgnoreRule(QString rule)
{
    IgnoreListManager::IgnoreListItem newItem = IgnoreListManager::IgnoreListItem();
    newItem.strictness = IgnoreListManager::SoftStrictness;
    newItem.scope = IgnoreListManager::GlobalScope;
    newItem.isRegEx = false;
    newItem.isActive = true;

    bool enableOkButton = false;
    if (!rule.isEmpty()) {
        // we're called from contextmenu
        newItem.ignoreRule = rule;
        enableOkButton = true;
    }

    IgnoreListEditDlg *dlg = new IgnoreListEditDlg(newItem, this, enableOkButton);
    dlg->enableOkButton(enableOkButton);
    while (dlg->exec() == QDialog::Accepted) {
        if (!_ignoreListModel.newIgnoreRule(dlg->ignoreListItem())) {
            if (QMessageBox::warning(this,
                    tr("Rule already exists"),
                    tr("There is already a rule\n\"%1\"\nPlease choose another rule.")
                    .arg(dlg->ignoreListItem().ignoreRule),
                    QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok)
                == QMessageBox::Cancel)
                break;

            IgnoreListManager::IgnoreListItem item = dlg->ignoreListItem();
            delete dlg;
            dlg = new IgnoreListEditDlg(item, this);
        }
        else {
            break;
        }
    }
    dlg->deleteLater();
}


void IgnoreListSettingsPage::editSelectedIgnoreRule()
{
    if (!ui.ignoreListView->selectionModel()->hasSelection())
        return;
    int row = ui.ignoreListView->selectionModel()->selectedIndexes()[0].row();
    IgnoreListEditDlg dlg(_ignoreListModel.ignoreListItemAt(row), this);
    dlg.setAttribute(Qt::WA_DeleteOnClose, false);
    if (dlg.exec() == QDialog::Accepted) {
        _ignoreListModel.setIgnoreListItemAt(row, dlg.ignoreListItem());
    }
}


void IgnoreListSettingsPage::editIgnoreRule(const QString &ignoreRule)
{
    ui.ignoreListView->selectionModel()->select(_ignoreListModel.indexOf(ignoreRule), QItemSelectionModel::Select);
    if (ui.ignoreListView->selectionModel()->hasSelection()) // && ui.ignoreListView->selectionModel()->selectedIndexes()[0].row() != -1)
        editSelectedIgnoreRule();
    else
        newIgnoreRule(ignoreRule);
}


/*
  IgnoreListDelegate
*/
void IgnoreListDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.column() == 0) {
        QStyle *style = QApplication::style();
        if (option.state & QStyle::State_Selected)
            painter->fillRect(option.rect, option.palette.highlight());

        QStyleOptionButton opts;
        opts.direction = option.direction;
        opts.rect = option.rect;
        opts.rect.moveLeft(option.rect.center().rx()-10);
        opts.state = option.state;
        opts.state |= index.data().toBool() ? QStyle::State_On : QStyle::State_Off;
        style->drawControl(QStyle::CE_CheckBox, &opts, painter);
    }
    else
        QStyledItemDelegate::paint(painter, option, index);
}


// provide interactivity for the checkboxes
bool IgnoreListDelegate::editorEvent(QEvent *event, QAbstractItemModel *model,
    const QStyleOptionViewItem &option, const QModelIndex &index)
{
    Q_UNUSED(option)
    switch (event->type()) {
    case QEvent::MouseButtonRelease:
        model->setData(index, !index.data().toBool());
        return true;
    // don't show the default editor for the column
    case QEvent::MouseButtonDblClick:
        return true;
    default:
        return false;
    }
}


/*
  IgnoreListEditDlg
*/
IgnoreListEditDlg::IgnoreListEditDlg(const IgnoreListManager::IgnoreListItem &item, QWidget *parent, bool enabled)
    : QDialog(parent), _ignoreListItem(item), _hasChanged(enabled)
{
    ui.setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setModal(true);
    // FIXME patch out the bugger completely if it's good without it
    ui.isActiveCheckBox->hide();

    // setup buttongroups
    // this could be moved to .ui file with qt4.5
    _typeButtonGroup.addButton(ui.senderTypeButton, 0);
    _typeButtonGroup.addButton(ui.messageTypeButton, 1);
    _typeButtonGroup.addButton(ui.ctcpTypeButton, 2);
    _strictnessButtonGroup.addButton(ui.dynamicStrictnessButton, 0);
    _strictnessButtonGroup.addButton(ui.permanentStrictnessButton, 1);
    _scopeButtonGroup.addButton(ui.globalScopeButton, 0);
    _scopeButtonGroup.addButton(ui.networkScopeButton, 1);
    _scopeButtonGroup.addButton(ui.channelScopeButton, 2);

    ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    ui.ignoreRuleLineEdit->setText(item.ignoreRule);

    if (item.type == IgnoreListManager::MessageIgnore)
        ui.messageTypeButton->setChecked(true);
    else if (item.type == IgnoreListManager::CtcpIgnore)
        ui.ctcpTypeButton->setChecked(true);
    else
        ui.senderTypeButton->setChecked(true);

    ui.isRegExCheckBox->setChecked(item.isRegEx);
    ui.isActiveCheckBox->setChecked(item.isActive);

    if (item.strictness == IgnoreListManager::HardStrictness)
        ui.permanentStrictnessButton->setChecked(true);
    else
        ui.dynamicStrictnessButton->setChecked(true);

    switch (item.scope) {
    case IgnoreListManager::NetworkScope:
        ui.networkScopeButton->setChecked(true);
        ui.scopeRuleTextEdit->setEnabled(true);
        break;
    case IgnoreListManager::ChannelScope:
        ui.channelScopeButton->setChecked(true);
        ui.scopeRuleTextEdit->setEnabled(true);
        break;
    default:
        ui.globalScopeButton->setChecked(true);
        ui.scopeRuleTextEdit->setEnabled(false);
    }

    if (item.scope == IgnoreListManager::GlobalScope)
        ui.scopeRuleTextEdit->clear();
    else
        ui.scopeRuleTextEdit->setPlainText(item.scopeRule);

    connect(ui.ignoreRuleLineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(widgetHasChanged()));
    connect(ui.scopeRuleTextEdit, SIGNAL(textChanged()), this, SLOT(widgetHasChanged()));
    connect(&_typeButtonGroup, SIGNAL(buttonClicked(int)), this, SLOT(widgetHasChanged()));
    connect(&_strictnessButtonGroup, SIGNAL(buttonClicked(int)), this, SLOT(widgetHasChanged()));
    connect(&_scopeButtonGroup, SIGNAL(buttonClicked(int)), this, SLOT(widgetHasChanged()));
    connect(ui.isRegExCheckBox, SIGNAL(stateChanged(int)), this, SLOT(widgetHasChanged()));
    connect(ui.isActiveCheckBox, SIGNAL(stateChanged(int)), this, SLOT(widgetHasChanged()));

    connect(ui.buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(aboutToAccept()));
    widgetHasChanged();
}


void IgnoreListEditDlg::widgetHasChanged()
{
    if (ui.messageTypeButton->isChecked())
        _clonedIgnoreListItem.type = IgnoreListManager::MessageIgnore;
    else if (ui.ctcpTypeButton->isChecked())
        _clonedIgnoreListItem.type = IgnoreListManager::CtcpIgnore;
    else
        _clonedIgnoreListItem.type = IgnoreListManager::SenderIgnore;

    if (ui.permanentStrictnessButton->isChecked())
        _clonedIgnoreListItem.strictness = IgnoreListManager::HardStrictness;
    else
        _clonedIgnoreListItem.strictness = IgnoreListManager::SoftStrictness;

    if (ui.networkScopeButton->isChecked()) {
        _clonedIgnoreListItem.scope = IgnoreListManager::NetworkScope;
        ui.scopeRuleTextEdit->setEnabled(true);
    }
    else if (ui.channelScopeButton->isChecked()) {
        _clonedIgnoreListItem.scope = IgnoreListManager::ChannelScope;
        ui.scopeRuleTextEdit->setEnabled(true);
    }
    else {
        _clonedIgnoreListItem.scope = IgnoreListManager::GlobalScope;
        ui.scopeRuleTextEdit->setEnabled(false);
    }

    if (_clonedIgnoreListItem.scope == IgnoreListManager::GlobalScope) {
        _clonedIgnoreListItem.scopeRule = QString();
    }
    else {
        QStringList text = ui.scopeRuleTextEdit->toPlainText().split(";", QString::SkipEmptyParts);
        QStringList::iterator it = text.begin();
        while (it != text.end()) {
            *it = it->trimmed();
            ++it;
        }

        _clonedIgnoreListItem.scopeRule = text.join("; ");
    }

    _clonedIgnoreListItem.ignoreRule = ui.ignoreRuleLineEdit->text();
    _clonedIgnoreListItem.isRegEx = ui.isRegExCheckBox->isChecked();
    _clonedIgnoreListItem.isActive = ui.isActiveCheckBox->isChecked();

    if (!_clonedIgnoreListItem.ignoreRule.isEmpty() && _clonedIgnoreListItem != _ignoreListItem)
        _hasChanged = true;
    else
        _hasChanged = false;
    ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(_hasChanged);
}


void IgnoreListEditDlg::enableOkButton(bool state)
{
    ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(state);
}
