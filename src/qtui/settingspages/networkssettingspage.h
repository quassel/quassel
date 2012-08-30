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

#ifndef NETWORKSSETTINGSPAGE_H
#define NETWORKSSETTINGSPAGE_H

#include <QPixmap>

#include "network.h"
#include "settingspage.h"
#include "clientidentity.h"

#include "ui_networkssettingspage.h"
#include "ui_networkadddlg.h"
#include "ui_networkeditdlg.h"
#include "ui_servereditdlg.h"
#include "ui_saveidentitiesdlg.h"

class NetworksSettingsPage : public SettingsPage
{
    Q_OBJECT

public:
    NetworksSettingsPage(QWidget *parent = 0);

    virtual inline bool needsCoreConnection() const { return true; }

    bool aboutToSave();

public slots:
    void save();
    void load();

private slots:
    void widgetHasChanged();
    void setWidgetStates();
    void coreConnectionStateChanged(bool);
    void networkConnectionStateChanged(Network::ConnectionState state);
    void networkConnectionError(const QString &msg);

    void displayNetwork(NetworkId);
    void setItemState(NetworkId, QListWidgetItem *item = 0);

    void clientNetworkAdded(NetworkId);
    void clientNetworkRemoved(NetworkId);
    void clientNetworkUpdated();

    void clientIdentityAdded(IdentityId);
    void clientIdentityRemoved(IdentityId);
    void clientIdentityUpdated();

#ifdef HAVE_SSL
    void sslUpdated();
#endif

    void on_networkList_itemSelectionChanged();
    void on_addNetwork_clicked();
    void on_deleteNetwork_clicked();
    void on_renameNetwork_clicked();
    void on_editIdentities_clicked();

    // void on_connectNow_clicked();

    void on_serverList_itemSelectionChanged();
    void on_addServer_clicked();
    void on_deleteServer_clicked();
    void on_editServer_clicked();
    void on_upServer_clicked();
    void on_downServer_clicked();

private:
    Ui::NetworksSettingsPage ui;

    NetworkId currentId;
    QHash<NetworkId, NetworkInfo> networkInfos;
    bool _ignoreWidgetChanges;
#ifdef HAVE_SSL
    CertIdentity *_cid;
#endif

    QPixmap connectedIcon, connectingIcon, disconnectedIcon;

    void reset();
    bool testHasChanged();
    QListWidgetItem *insertNetwork(NetworkId);
    QListWidgetItem *insertNetwork(const NetworkInfo &info);
    QListWidgetItem *networkItem(NetworkId) const;
    void saveToNetworkInfo(NetworkInfo &);
    IdentityId defaultIdentity() const;
};


class NetworkAddDlg : public QDialog
{
    Q_OBJECT

public:
    NetworkAddDlg(const QStringList &existing = QStringList(), QWidget *parent = 0);

    NetworkInfo networkInfo() const;

private slots:
    void setButtonStates();

private:
    Ui::NetworkAddDlg ui;

    QStringList existing;
};


class NetworkEditDlg : public QDialog
{
    Q_OBJECT

public:
    NetworkEditDlg(const QString &old, const QStringList &existing = QStringList(), QWidget *parent = 0);

    QString networkName() const;

private slots:
    void on_networkEdit_textChanged(const QString &);

private:
    Ui::NetworkEditDlg ui;

    QStringList existing;
};


class ServerEditDlg : public QDialog
{
    Q_OBJECT

public:
    ServerEditDlg(const Network::Server &server = Network::Server(), QWidget *parent = 0);

    Network::Server serverData() const;

private slots:
    void on_host_textChanged();

private:
    Ui::ServerEditDlg ui;
};


class SaveNetworksDlg : public QDialog
{
    Q_OBJECT

public:
    SaveNetworksDlg(const QList<NetworkInfo> &toCreate, const QList<NetworkInfo> &toUpdate, const QList<NetworkId> &toRemove, QWidget *parent = 0);

private slots:
    void clientEvent();

private:
    Ui::SaveIdentitiesDlg ui;

    int numevents, rcvevents;
};


#endif
