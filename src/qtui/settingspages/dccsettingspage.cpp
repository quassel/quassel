/***************************************************************************
 *   Copyright (C) 2005-2022 by the Quassel Project                        *
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

#include "dccsettingspage.h"

#include "client.h"
#include "clienttransfermanager.h"
#include "util.h"

DccSettingsPage::DccSettingsPage(QWidget* parent)
    : SettingsPage(tr("IRC"), tr("DCC"), parent)
{
    ui.setupUi(this);
    initAutoWidgets();
    connect(ui.ipDetectionMode, selectOverload<int>(&QComboBox::currentIndexChanged), this, &DccSettingsPage::updateWidgetStates);
    connect(ui.portSelectionMode, selectOverload<int>(&QComboBox::currentIndexChanged), this, &DccSettingsPage::updateWidgetStates);
    updateWidgetStates();

    connect(Client::instance(), &Client::coreConnectionStateChanged, this, &DccSettingsPage::onClientConfigChanged);
    setClientConfig(Client::dccConfig());
}

bool DccSettingsPage::isClientConfigValid() const
{
    return _clientConfig != nullptr;
}

void DccSettingsPage::setClientConfig(DccConfig* config)
{
    if (_clientConfig) {
        disconnect(_clientConfig, nullptr, this, nullptr);
    }
    if (config && !isClientConfigValid()) {
        qWarning() << "Client DCC config is not valid/synchronized!";
        _clientConfig = nullptr;
        ui.dccEnabled->setEnabled(false);
        return;
    }
    _clientConfig = config;
    if (_clientConfig) {
        connect(_clientConfig, &DccConfig::updated, this, &DccSettingsPage::load);
        load();
        ui.dccEnabled->setEnabled(true);
    }
    else {
        ui.dccEnabled->setEnabled(false);
    }
}

void DccSettingsPage::onClientConfigChanged()
{
    if (Client::isConnected() && Client::dccConfig() && !Client::dccConfig()->isInitialized()) {
        connect(Client::dccConfig(), &SyncableObject::initDone, this, &DccSettingsPage::onClientConfigChanged);
    }
    else {
        setClientConfig(Client::isConnected() ? Client::dccConfig() : nullptr);
    }
}

bool DccSettingsPage::hasDefaults() const
{
    return true;
}

void DccSettingsPage::defaults()
{
    _localConfig.fromVariantMap(DccConfig{}.toVariantMap());
    SettingsPage::load();
    widgetHasChanged();
}

void DccSettingsPage::load()
{
    _localConfig.fromVariantMap(isClientConfigValid() ? _clientConfig->toVariantMap() : DccConfig{}.toVariantMap());
    SettingsPage::load();
    widgetHasChanged();
}

void DccSettingsPage::save()
{
    SettingsPage::save();
    if (isClientConfigValid()) {
        Client::dccConfig()->requestUpdate(_localConfig.toVariantMap());
    }
    setChangedState(false);
}

QVariant DccSettingsPage::loadAutoWidgetValue(const QString& widgetName)
{
    if (widgetName == "dccEnabled")
        return _localConfig.isDccEnabled();
    if (widgetName == "ipDetectionMode")
        // NOTE: Use mapping if item order differs from enum order
        return static_cast<int>(_localConfig.ipDetectionMode());
    if (widgetName == "portSelectionMode")
        // NOTE: Use mapping if item order differs from enum order
        return static_cast<int>(_localConfig.portSelectionMode());
    if (widgetName == "minPort")
        return _localConfig.minPort();
    if (widgetName == "maxPort")
        return _localConfig.maxPort();
    if (widgetName == "chunkSize")
        return _localConfig.chunkSize();
    if (widgetName == "sendTimeout")
        return _localConfig.sendTimeout();
    if (widgetName == "usePassiveDcc")
        return _localConfig.usePassiveDcc();
    if (widgetName == "useFastSend")
        return _localConfig.useFastSend();
    if (widgetName == "outgoingIp")
        return _localConfig.outgoingIp().toString();

    qWarning() << "Unknown auto widget" << widgetName;
    return {};
}

void DccSettingsPage::saveAutoWidgetValue(const QString& widgetName, const QVariant& value)
{
    if (widgetName == "dccEnabled")
        _localConfig.setDccEnabled(value.toBool());
    else if (widgetName == "ipDetectionMode")
        // NOTE: Use mapping if item order differs from enum order
        _localConfig.setIpDetectionMode(static_cast<DccConfig::IpDetectionMode>(value.toInt()));
    else if (widgetName == "portSelectionMode")
        // NOTE: Use mapping if item order differs from enum order
        _localConfig.setPortSelectionMode(static_cast<DccConfig::PortSelectionMode>(value.toInt()));
    else if (widgetName == "minPort")
        _localConfig.setMinPort(value.toInt());
    else if (widgetName == "maxPort")
        _localConfig.setMaxPort(value.toInt());
    else if (widgetName == "chunkSize")
        _localConfig.setChunkSize(value.toInt());
    else if (widgetName == "sendTimeout")
        _localConfig.setSendTimeout(value.toInt());
    else if (widgetName == "usePassiveDcc")
        _localConfig.setUsePassiveDcc(value.toBool());
    else if (widgetName == "useFastSend")
        _localConfig.setUseFastSend(value.toBool());
    else if (widgetName == "outgoingIp") {
        QHostAddress address{QHostAddress::LocalHost};
        if (!address.setAddress(value.toString())) {
            qWarning() << "Invalid IP address!";
            address = QHostAddress{QHostAddress::LocalHost};
        }
        _localConfig.setOutgoingIp(std::move(address));
    }
    else {
        qWarning() << "Unknown auto widget" << widgetName;
    }
}

void DccSettingsPage::widgetHasChanged()
{
    bool same = isClientConfigValid() && (_localConfig == *_clientConfig);
    setChangedState(!same);
}

void DccSettingsPage::updateWidgetStates()
{
    ui.outgoingIp->setEnabled(ui.ipDetectionMode->currentIndex() != 0);
    bool enablePorts = ui.portSelectionMode->currentIndex() != 0;
    ui.minPort->setEnabled(enablePorts);
    ui.maxPort->setEnabled(enablePorts);
    ui.portsToLabel->setEnabled(enablePorts);
}
