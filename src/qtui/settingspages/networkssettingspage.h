/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel IRC Team                         *
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

#ifndef _NETWORKSSETTINGSPAGE_H_
#define _NETWORKSSETTINGSPAGE_H_

#include <QIcon>

#include "settingspage.h"
#include "ui_networkssettingspage.h"
#include "ui_networkeditdlgnew.h"

#include "network.h"
#include "types.h"

class NetworksSettingsPage : public SettingsPage {
  Q_OBJECT

  public:
    NetworksSettingsPage(QWidget *parent = 0);

    //bool aboutToSave();

  public slots:
    void save();
    void load();

  private slots:
    void widgetHasChanged();
    void setWidgetStates();
    void coreConnectionStateChanged(bool);

    void displayNetwork(NetworkId, bool dontsave = false);

    void clientNetworkAdded(NetworkId);
    void clientNetworkUpdated();

    void clientIdentityAdded(IdentityId);
    void clientIdentityRemoved(IdentityId);
    void clientIdentityUpdated();

    void on_networkList_itemSelectionChanged();
    void on_addNetwork_clicked();

  private:
    Ui::NetworksSettingsPage ui;

    NetworkId currentId;
    QHash<NetworkId, NetworkInfo> networkInfos;

    QIcon connectedIcon, disconnectedIcon;

    void reset();
    bool testHasChanged();
    void insertNetwork(NetworkId);
    QListWidgetItem *networkItem(NetworkId) const;
};

class NetworkEditDlgNew : public QDialog {
  Q_OBJECT

  public:
    NetworkEditDlgNew(const QString &old, const QStringList &existing = QStringList(), QWidget *parent = 0);

    QString networkName() const;

  private slots:
    void on_networkEdit_textChanged(const QString &);

  private:
    Ui::NetworkEditDlgNew ui;

    QStringList existing;
};



class ServerEditDlgNew : public QDialog {
  Q_OBJECT

  public:
    ServerEditDlgNew(const QVariantMap &serverData = QVariantMap(), QWidget *parent = 0);

    QVariantMap serverData() const;

  private:
    QVariantMap _serverData;
};



#endif
