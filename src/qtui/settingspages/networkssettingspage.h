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

#ifndef NETWORKSSETTINGSPAGE_H
#define NETWORKSSETTINGSPAGE_H

#include <QIcon>

#include "clientidentity.h"
#include "network.h"
#include "settingspage.h"

#include "ui_capseditdlg.h"
#include "ui_networkadddlg.h"
#include "ui_networkeditdlg.h"
#include "ui_networkssettingspage.h"
#include "ui_saveidentitiesdlg.h"
#include "ui_servereditdlg.h"

class NetworksSettingsPage : public SettingsPage
{
    Q_OBJECT

public:
    NetworksSettingsPage(QWidget* parent = nullptr);

    inline bool needsCoreConnection() const override { return true; }

    bool aboutToSave() override;

public slots:
    void save() override;
    void load() override;
    void bufferList_Open(NetworkId);

private slots:
    void widgetHasChanged();
    void setWidgetStates();
    void coreConnectionStateChanged(bool);
    void networkConnectionStateChanged(Network::ConnectionState state);
    void networkConnectionError(const QString& msg);

    void displayNetwork(NetworkId);
    void setItemState(NetworkId, QListWidgetItem* item = nullptr);

    /**
     * Reset the capability-dependent settings to the default unknown states
     *
     * For example, this updates the SASL text to indicate the status is unknown.  Any actual
     * information should be set by setNetworkCapStates()
     *
     * @see NetworksSettingsPage::setNetworkCapStates()
     */
    void resetNetworkCapStates();

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

    void sslUpdated();

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

    /**
     * Event handler for Features status Details button
     */
    void on_enableCapsStatusDetails_clicked();

    /**
     * Event handler for Features Advanced edit button
     */
    void on_enableCapsAdvanced_clicked();

private:
    /**
     * Status of capability support
     */
    enum CapSupportStatus
    {
        Unknown,           ///< Old core, or otherwise unknown, can't make assumptions
        Disconnected,      ///< Disconnected from network, can't determine
        MaybeUnsupported,  ///< Server does not advertise support at this moment
        MaybeSupported     ///< Server advertises support at this moment
    };
    // Keep in mind networks can add, change, and remove capabilities at any time.

    Ui::NetworksSettingsPage ui;

    NetworkId currentId;
    QHash<NetworkId, NetworkInfo> networkInfos;
    bool _ignoreWidgetChanges{false};
    CertIdentity* _cid{nullptr};

    QIcon connectedIcon, connectingIcon, disconnectedIcon;

    // Status icons
    QIcon infoIcon, successIcon, unavailableIcon, questionIcon;

    CapSupportStatus _capSaslStatusSelected;  ///< Status of SASL support for selected network
    bool _capSaslStatusUsingExternal{false};  ///< Whether SASL support status is for SASL EXTERNAL

    void reset();
    bool testHasChanged();
    QListWidgetItem* insertNetwork(NetworkId);
    QListWidgetItem* insertNetwork(const NetworkInfo& info);
    QListWidgetItem* networkItem(NetworkId) const;
    void saveToNetworkInfo(NetworkInfo&);
    IdentityId defaultIdentity() const;

    /**
     * Get whether or not the displayed network's identity has SSL certs associated with it
     *
     * @return True if the currently displayed network has SSL certs set, otherwise false
     */
    bool displayedNetworkHasCertId() const;

    /**
     * Update the SASL settings interface according to the given SASL state
     *
     * @param saslStatus         Current status of SASL support.
     * @param usingSASLExternal  If true, SASL support status is for SASL EXTERNAL, else SASL PLAIN
     */
    void setCapSASLStatus(const CapSupportStatus saslStatus, bool usingSASLExternal = false);
};

class NetworkAddDlg : public QDialog
{
    Q_OBJECT

public:
    NetworkAddDlg(QStringList existing = QStringList(), QWidget* parent = nullptr);

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
    NetworkEditDlg(const QString& old, QStringList existing = QStringList(), QWidget* parent = nullptr);

    QString networkName() const;

private slots:
    void on_networkEdit_textChanged(const QString&);

private:
    Ui::NetworkEditDlg ui;

    QStringList existing;
};

class ServerEditDlg : public QDialog
{
    Q_OBJECT

public:
    ServerEditDlg(const Network::Server& server = Network::Server(), QWidget* parent = nullptr);

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

class CapsEditDlg : public QDialog
{
    Q_OBJECT

public:
    CapsEditDlg(const QString& oldSkipCapsString, QWidget* parent = nullptr);

    QString skipCapsString() const;

private slots:
    void defaultSkipCaps();
    void on_skipCapsEdit_textChanged(const QString&);

private:
    Ui::CapsEditDlg ui;

    QString oldSkipCapsString;
};

class SaveNetworksDlg : public QDialog
{
    Q_OBJECT

public:
    SaveNetworksDlg(const QList<NetworkInfo>& toCreate,
                    const QList<NetworkInfo>& toUpdate,
                    const QList<NetworkId>& toRemove,
                    QWidget* parent = nullptr);

private slots:
    void clientEvent();

private:
    Ui::SaveIdentitiesDlg ui;

    int numevents, rcvevents;
};

#endif
