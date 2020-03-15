/***************************************************************************
 *   Copyright (C) 2005-2020 by the Quassel Project                        *
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

#include <QSortFilterProxyModel>

#include "irclisthelper.h"
#include "irclistmodel.h"
#include "types.h"

#include "ui_channellistdlg.h"

class QSpacerItem;

class ChannelListDlg : public QDialog
{
    Q_OBJECT

public:
    ChannelListDlg(QWidget* parent = nullptr);

    void setNetwork(NetworkId netId);

    /**
     * Set the channel search string, enabling advanced mode if needed
     *
     * Sets the channel name search text to the specified string, enabling advanced mode.  If search
     * string is empty, advanced mode will be automatically hidden.
     *
     * @param channelFilters Partial channel name to search for, or empty to not filter by name
     */
    void setChannelFilters(const QString& channelFilters);

public slots:
    /**
     * Request a channel listing using any parameters set in the UI
     */
    void requestSearch();

protected slots:
    void receiveChannelList(const NetworkId& netId,
                            const QStringList& channelFilters,
                            const QList<IrcListHelper::ChannelDescription>& channelList);
    void reportFinishedList();
    void joinChannel(const QModelIndex&);

private slots:
    inline void toggleMode() { setAdvancedMode(!_advancedMode); }
    void showError(const QString& error);

private:
    void showFilterLine(bool show);
    void showErrors(bool show);
    void enableQuery(bool enable);
    void setAdvancedMode(bool advanced);

    /**
     * Update the focus of input widgets according to dialog state
     */
    void updateInputFocus();

    Ui::ChannelListDlg ui;

    bool _listFinished{true};
    NetworkId _netId;
    IrcListModel _ircListModel;
    QSortFilterProxyModel _sortFilter;
    QSpacerItem* _simpleModeSpacer{nullptr};
    bool _advancedMode{false};
};

#endif  // CHANNELLIST_H
