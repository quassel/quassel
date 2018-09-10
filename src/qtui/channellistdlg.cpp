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

#include "channellistdlg.h"

#include <QHeaderView>
#include <QHBoxLayout>
#include <QSpacerItem>

#include "client.h"
#include "clientirclisthelper.h"
#include "icon.h"

ChannelListDlg::ChannelListDlg(QWidget *parent)
    : QDialog(parent),
    _ircListModel(this),
    _sortFilter(this)
{
    _sortFilter.setSourceModel(&_ircListModel);
    _sortFilter.setFilterCaseSensitivity(Qt::CaseInsensitive);
    _sortFilter.setFilterKeyColumn(-1);

    ui.setupUi(this);
    ui.advancedModeLabel->setPixmap(icon::get("edit-rename").pixmap(22));

    ui.channelListView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui.channelListView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui.channelListView->setAlternatingRowColors(true);
    ui.channelListView->setTabKeyNavigation(false);
    ui.channelListView->setModel(&_sortFilter);
    ui.channelListView->setSortingEnabled(true);
    // Sort A-Z by default
    ui.channelListView->sortByColumn(0, Qt::AscendingOrder);
    ui.channelListView->verticalHeader()->hide();
    ui.channelListView->horizontalHeader()->setStretchLastSection(true);

    ui.searchChannelsButton->setAutoDefault(false);

    setWindowIcon(icon::get("format-list-unordered"));

    connect(ui.advancedModeLabel, &ClickableLabel::clicked, this, &ChannelListDlg::toggleMode);
    connect(ui.searchChannelsButton, &QAbstractButton::clicked, this, &ChannelListDlg::requestSearch);
    connect(ui.channelNameLineEdit, &QLineEdit::returnPressed, this, &ChannelListDlg::requestSearch);
    connect(ui.filterLineEdit, &QLineEdit::textChanged, &_sortFilter, &QSortFilterProxyModel::setFilterFixedString);
    connect(Client::ircListHelper(), &ClientIrcListHelper::channelListReceived,
        this, &ChannelListDlg::receiveChannelList);
    connect(Client::ircListHelper(), &ClientIrcListHelper::finishedListReported, this, &ChannelListDlg::reportFinishedList);
    connect(Client::ircListHelper(), &ClientIrcListHelper::errorReported, this, &ChannelListDlg::showError);
    connect(ui.channelListView, &QAbstractItemView::activated, this, &ChannelListDlg::joinChannel);

    setAdvancedMode(false);
    enableQuery(true);
    showFilterLine(false);
    showErrors(false);

    // Set initial input focus
    updateInputFocus();
}


void ChannelListDlg::setNetwork(NetworkId netId)
{
    if (_netId == netId)
        return;

    _netId = netId;
    _ircListModel.setChannelList();
    showFilterLine(false);
}


void ChannelListDlg::setChannelFilters(const QString &channelFilters)
{
    // Enable advanced mode if searching
    setAdvancedMode(!channelFilters.isEmpty());
    // Set channel search text after setting advanced mode so it's not cleared
    ui.channelNameLineEdit->setText(channelFilters.trimmed());
}


void ChannelListDlg::requestSearch()
{
    if (!_netId.isValid()) {
        // No valid network set yet
        return;
    }

    _listFinished = false;
    enableQuery(false);
    showErrors(false);
    QStringList channelFilters;
    channelFilters << ui.channelNameLineEdit->text().trimmed();
    Client::ircListHelper()->requestChannelList(_netId, channelFilters);
}


void ChannelListDlg::receiveChannelList(const NetworkId &netId, const QStringList &channelFilters, const QList<IrcListHelper::ChannelDescription> &channelList)
{
    Q_UNUSED(channelFilters)
    if (netId != _netId)
        return;

    showFilterLine(!channelList.isEmpty());
    _ircListModel.setChannelList(channelList);
    enableQuery(_listFinished);
    // Reset input focus since UI changed
    updateInputFocus();
}


void ChannelListDlg::showFilterLine(bool show)
{
    ui.line->setVisible(show);
    ui.filterLabel->setVisible(show);
    ui.filterLineEdit->setVisible(show);
}


void ChannelListDlg::enableQuery(bool enable)
{
    ui.channelNameLineEdit->setEnabled(enable);
    ui.searchChannelsButton->setEnabled(enable);
}


void ChannelListDlg::setAdvancedMode(bool advanced)
{
    _advancedMode = advanced;

    if (advanced) {
        if (_simpleModeSpacer) {
            ui.searchLayout->removeItem(_simpleModeSpacer);
            delete _simpleModeSpacer;
            _simpleModeSpacer = nullptr;
        }
        ui.advancedModeLabel->setPixmap(icon::get("edit-clear-locationbar-rtl").pixmap(16));
    }
    else {
        if (!_simpleModeSpacer) {
            _simpleModeSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
            ui.searchLayout->insertSpacerItem(0, _simpleModeSpacer);
        }
        ui.advancedModeLabel->setPixmap(icon::get("edit-rename").pixmap(16));
    }

    ui.channelNameLineEdit->clear();
    ui.channelNameLineEdit->setVisible(advanced);
    ui.searchPatternLabel->setVisible(advanced);
}


void ChannelListDlg::updateInputFocus()
{
    // Update keyboard focus to match what options are available.  Prioritize the channel name
    // editor as one likely won't need to filter when already limiting the list.
    if (ui.channelNameLineEdit->isVisible()) {
        ui.channelNameLineEdit->setFocus();
    } else if (ui.filterLineEdit->isVisible()) {
        ui.filterLineEdit->setFocus();
    }
}


void ChannelListDlg::showErrors(bool show)
{
    if (!show) {
        ui.errorTextEdit->clear();
    }
    ui.errorLabel->setVisible(show);
    ui.errorTextEdit->setVisible(show);
}


void ChannelListDlg::reportFinishedList()
{
    _listFinished = true;
}


void ChannelListDlg::showError(const QString &error)
{
    showErrors(true);
    ui.errorTextEdit->moveCursor(QTextCursor::End);
    ui.errorTextEdit->insertPlainText(error + "\n");
}


void ChannelListDlg::joinChannel(const QModelIndex &index)
{
    Client::instance()->userInput(BufferInfo::fakeStatusBuffer(_netId), QString("/JOIN %1").arg(index.sibling(index.row(), 0).data().toString()));
}
