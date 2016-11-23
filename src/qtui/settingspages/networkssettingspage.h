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

#ifndef NETWORKSSETTINGSPAGE_H
#define NETWORKSSETTINGSPAGE_H

#include <QIcon>

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
    void bufferList_Open(NetworkId);

private slots:
    void widgetHasChanged();
    void setWidgetStates();
    void coreConnectionStateChanged(bool);
    void networkConnectionStateChanged(Network::ConnectionState state);
    void networkConnectionError(const QString &msg);

    void displayNetwork(NetworkId);
    void setItemState(NetworkId, QListWidgetItem *item = 0);

    /**
     * Update the capability-dependent settings according to what the server supports
     *
     * For example, this updates the SASL text for when the server advertises support.  This should
     * only be called on the currently displayed network.
     *
     * @param[in] id  NetworkId referencing network used to update settings user interface.
     */
    void setNetworkCapStates(NetworkId id);

    void clientNetworkAdded(NetworkId);
    void clientNetworkRemoved(NetworkId);
    void clientNetworkUpdated();

    void clientIdentityAdded(IdentityId);
    void clientIdentityRemoved(IdentityId);
    void clientIdentityUpdated();

    /**
     * Update the settings user interface according to capabilities advertised by the IRC server
     */
    void clientNetworkCapsUpdated();

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

    /**
     * Event handler for SASL status Details button
     */
    void on_saslStatusDetails_clicked();

private:
    /**
     * Status of capability support
     */
    enum CapSupportStatus {
        Unknown,           ///< Old core, or otherwise unknown, can't make assumptions
        Disconnected,      ///< Disconnected from network, can't determine
        MaybeUnsupported,  ///< Server does not advertise support at this moment
        MaybeSupported     ///< Server advertises support at this moment
    };
    // Keep in mind networks can add, change, and remove capabilities at any time.

    Ui::NetworksSettingsPage ui;

    NetworkId currentId;
    QHash<NetworkId, NetworkInfo> networkInfos;
    bool _ignoreWidgetChanges;
#ifdef HAVE_SSL
    CertIdentity *_cid;
#endif

    QIcon connectedIcon, connectingIcon, disconnectedIcon;

    // Status icons
    QIcon infoIcon, warningIcon;

    CapSupportStatus _saslStatusSelected;  /// Status of SASL support for currently-selected network

    void reset();
    bool testHasChanged();
    QListWidgetItem *insertNetwork(NetworkId);
    QListWidgetItem *insertNetwork(const NetworkInfo &info);
    QListWidgetItem *networkItem(NetworkId) const;
    void saveToNetworkInfo(NetworkInfo &);
    IdentityId defaultIdentity() const;

    /**
     * Update the SASL settings interface according to the given SASL state
     *
     * @param[in] saslStatus Current status of SASL support.
     */
    void setSASLStatus(const CapSupportStatus saslStatus);
};


class NetworkAddDlg : public QDialog
{
    Q_OBJECT

public:
    NetworkAddDlg(const QStringList &existing = QStringList(), QWidget *parent = 0);

    NetworkInfo networkInfo() const;

private slots:
    void setButtonStates();

    /**
     * Update the default server port according to isChecked
     *
     * Connect with useSSL->toggled() in order to keep the port number in sync.  This only modifies
     * the port if it's not been changed from defaults.
     *
     * @param isChecked If true and port unchanged, set port to 6697, else set port to 6667.
     */
    void updateSslPort(bool isChecked);

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

    /**
     * Update the default server port according to isChecked
     *
     * Connect with useSSL->toggled() in order to keep the port number in sync.  This only modifies
     * the port if it's not been changed from defaults.
     *
     * @param isChecked If true and port unchanged, set port to 6697, else set port to 6667.
     */
    void updateSslPort(bool isChecked);

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
