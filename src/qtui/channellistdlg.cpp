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

#include "channellistdlg.h"

#include "client.h"
#include "clientirclisthelper.h"

#include <QHeaderView>
#include <QHBoxLayout>
#include <QSpacerItem>

ChannelListDlg::ChannelListDlg(QWidget *parent)
  : QDialog(parent),
    _listFinished(true),
    _ircListModel(this),
    _sortFilter(this),
    _simpleModeSpacer(0),
    _advancedMode(false)
{
  _sortFilter.setSourceModel(&_ircListModel);
  _sortFilter.setFilterCaseSensitivity(Qt::CaseInsensitive);
  _sortFilter.setFilterKeyColumn(-1);
  
  ui.setupUi(this);
  ui.channelListView->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui.channelListView->setSelectionMode(QAbstractItemView::SingleSelection);
  ui.channelListView->setAlternatingRowColors(true);
  ui.channelListView->setTabKeyNavigation(false);
  ui.channelListView->setModel(&_sortFilter);
  ui.channelListView->setSortingEnabled(true);
  ui.channelListView->verticalHeader()->hide();
  ui.channelListView->horizontalHeader()->setStretchLastSection(true);

  ui.searchChannelsButton->setAutoDefault(false);

  connect(ui.advancedModeLabel, SIGNAL(clicked()), this, SLOT(toggleMode()));
  connect(ui.searchChannelsButton, SIGNAL(clicked()), this, SLOT(requestSearch()));
  connect(ui.channelNameLineEdit, SIGNAL(returnPressed()), this, SLOT(requestSearch()));
  connect(ui.filterLineEdit, SIGNAL(textChanged(QString)), &_sortFilter, SLOT(setFilterFixedString(QString)));
  connect(Client::ircListHelper(), SIGNAL(channelListReceived(const NetworkId &, const QStringList &, QList<IrcListHelper::ChannelDescription>)),
	  this, SLOT(receiveChannelList(NetworkId , QStringList, QList<IrcListHelper::ChannelDescription>)));
  connect(Client::ircListHelper(), SIGNAL(finishedListReported(const NetworkId &)), this, SLOT(reportFinishedList()));
  connect(Client::ircListHelper(), SIGNAL(errorReported(const QString &)), this, SLOT(showError(const QString &)));
  connect(ui.channelListView, SIGNAL(activated(QModelIndex)), this, SLOT(joinChannel(QModelIndex)));

  setAdvancedMode(false);
  enableQuery(true);
  showFilterLine(false);
  showErrors(false);
}

void ChannelListDlg::setNetwork(NetworkId netId) {
  if(_netId == netId)
    return;
  
  _netId = netId;
  _ircListModel.setChannelList();
  showFilterLine(false);
}

void ChannelListDlg::requestSearch() {
  _listFinished = false;
  enableQuery(false);
  showErrors(false);
  QStringList channelFilters;
  channelFilters << ui.channelNameLineEdit->text().trimmed();
  Client::ircListHelper()->requestChannelList(_netId, channelFilters);
}

void ChannelListDlg::receiveChannelList(const NetworkId &netId, const QStringList &channelFilters, const QList<IrcListHelper::ChannelDescription> &channelList) {
  Q_UNUSED(netId)
  Q_UNUSED(channelFilters)

  showFilterLine(!channelList.isEmpty());
  _ircListModel.setChannelList(channelList);
  enableQuery(_listFinished);
}

void ChannelListDlg::showFilterLine(bool show) {
  ui.line->setVisible(show);
  ui.filterLabel->setVisible(show);
  ui.filterLineEdit->setVisible(show);
}

void ChannelListDlg::enableQuery(bool enable) {
  ui.channelNameLineEdit->setEnabled(enable);
  ui.searchChannelsButton->setEnabled(enable);
}

void ChannelListDlg::setAdvancedMode(bool advanced) {
  _advancedMode = advanced;

  QHBoxLayout *searchLayout = 0;
#if QT_VERSION >=  0x040400
  searchLayout = ui.searchLayout;
#else
  // FIXME: REMOVE WHEN WE DEPEND ON Qt 4.4
  /*
   * ok this just sucks: in Qt 4.3 there is no way to search for a layout as uic creates
   * them without a parent -.-
   * in this case there are only 2 candidates: ui.hboxLayout and ui.hboxLayout1
   */
  if(ui.hboxLayout.findWidget(ui.searchPatternLabel) != -1)
    searchLayout = ui.hboxLayout;
  else if(ui.hboxLayout1.findWidget(ui.searchPatternLabel) != -1)
    searchLayout = ui.hboxLayout1;
  else
    /* if this assert trigger we have been compiled on a too old Qt
     * or uic generated something very unexpected. we cannot find the layout to manipulate.
     * Please upgrade to a recent version of Qt.
     */
    Q_ASSERT(false);
#endif

  if(advanced) {
    if(_simpleModeSpacer) {
      searchLayout->removeItem(_simpleModeSpacer);
      delete _simpleModeSpacer;
      _simpleModeSpacer = 0;
    }
    ui.advancedModeLabel->setPixmap(QPixmap(QString::fromUtf8(":/22x22/actions/oxygen/22x22/actions/edit-clear-locationbar-rtl.png")));
  } else {
    if(!_simpleModeSpacer) {
      _simpleModeSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
      searchLayout->insertSpacerItem(0, _simpleModeSpacer);
    }
    ui.advancedModeLabel->setPixmap(QPixmap(QString::fromUtf8(":/22x22/actions/oxygen/22x22/actions/edit-clear.png")));
  }
  ui.channelNameLineEdit->clear();
  ui.channelNameLineEdit->setVisible(advanced);
  ui.searchPatternLabel->setVisible(advanced);
}

void ChannelListDlg::showErrors(bool show) {
  if(!show) {
    ui.errorTextEdit->clear();
  }
  ui.errorLabel->setVisible(show);
  ui.errorTextEdit->setVisible(show);
}


void ChannelListDlg::reportFinishedList() {
  _listFinished = true;
}

void ChannelListDlg::showError(const QString &error) {
  showErrors(true);
  ui.errorTextEdit->moveCursor(QTextCursor::End);
  ui.errorTextEdit->insertPlainText(error + "\n");
}

void ChannelListDlg::joinChannel(const QModelIndex &index) {
  Client::instance()->userInput(BufferInfo::fakeStatusBuffer(_netId), QString("/JOIN %1").arg(index.sibling(index.row(), 0).data().toString()));
}
