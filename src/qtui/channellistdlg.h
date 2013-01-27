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

#ifndef CHANNELLISTDLG_H
#define CHANNELLISTDLG_H

#include "ui_channellistdlg.h"

#include "irclisthelper.h"
#include "irclistmodel.h"
#include "types.h"

#include <QSortFilterProxyModel>

class QSpacerItem;

class ChannelListDlg : public QDialog
{
    Q_OBJECT

public:
    ChannelListDlg(QWidget *parent = 0);

    void setNetwork(NetworkId netId);

protected slots:
    void requestSearch();
    void receiveChannelList(const NetworkId &netId, const QStringList &channelFilters, const QList<IrcListHelper::ChannelDescription> &channelList);
    void reportFinishedList();
    void joinChannel(const QModelIndex &);

private slots:
    inline void toggleMode() { setAdvancedMode(!_advancedMode); }
    void showError(const QString &error);

private:
    void showFilterLine(bool show);
    void showErrors(bool show);
    void enableQuery(bool enable);
    void setAdvancedMode(bool advanced);

    Ui::ChannelListDlg ui;

    bool _listFinished;
    NetworkId _netId;
    IrcListModel _ircListModel;
    QSortFilterProxyModel _sortFilter;
    QSpacerItem *_simpleModeSpacer;
    bool _advancedMode;
};


#endif //CHANNELLIST_H
