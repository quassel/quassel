/***************************************************************************
 *   Copyright (C) 2005-2019 by the Quassel Project                        *
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

#include "coreinfodlg.h"

#include <QMessageBox>

#include "bufferwidget.h"
#include "client.h"
#include "icon.h"
#include "util.h"

CoreInfoDlg::CoreInfoDlg(QWidget* parent)
    : QDialog(parent)
{
    ui.setupUi(this);

    // Listen for resynchronization events (pre-0.13 cores only)
    connect(Client::instance(), &Client::coreInfoResynchronized, this, &CoreInfoDlg::coreInfoResynchronized);

    // Update legacy core info for Quassel cores earlier than 0.13.  This does nothing on modern
    // cores.
    refreshLegacyCoreInfo();

    // Display existing core info, set up signal handlers
    coreInfoResynchronized();

    // Warning icon
    ui.coreUnsupportedIcon->setPixmap(icon::get("dialog-warning").pixmap(16));

    updateUptime();
    startTimer(1000);
}

void CoreInfoDlg::refreshLegacyCoreInfo()
{
    if (!Client::isConnected() || Client::isCoreFeatureEnabled(Quassel::Feature::SyncedCoreInfo)) {
        // If we're not connected, or the core supports SyncedCoreInfo (0.13+), bail out
        return;
    }

    // Request legacy (pre-0.13) CoreInfo object to be resynchronized (does nothing on modern cores)
    Client::refreshLegacyCoreInfo();

    // On legacy cores, CoreInfo data does not send signals.  Periodically poll for information.
    // 15 seconds seems like a reasonable trade-off as this only happens while the dialog is open.
    QTimer::singleShot(15 * 1000, this, &CoreInfoDlg::refreshLegacyCoreInfo);
}

void CoreInfoDlg::coreInfoResynchronized()
{
    // CoreInfo object has been recreated, or this is the first time the dialog's been shown

    CoreInfo* coreInfo = Client::coreInfo();
    // Listen for changes to core information
    connect(coreInfo, &CoreInfo::coreDataChanged, this, &CoreInfoDlg::coreInfoChanged);

    // Update with any known core information set before connecting the signal.  This is needed for
    // both modern (0.13+) and legacy cores.
    coreInfoChanged(coreInfo->coreData());
}

void CoreInfoDlg::coreInfoChanged(const QVariantMap& coreInfo)
{
    if (coreInfo.isEmpty()) {
        // We're missing data for some reason
        if (Client::isConnected()) {
            // Core info is entirely empty despite being connected.  Something's probably wrong.
            ui.labelCoreVersion->setText(tr("Unknown"));
            ui.labelCoreVersionDate->setText(tr("Unknown"));
        }
        else {
            // We're disconnected.  Mark as such.
            ui.labelCoreVersion->setText(tr("Disconnected from core"));
            ui.labelCoreVersionDate->setText(tr("Not available"));
        }
        ui.labelClientCount->setNum(0);
        // Don't return, allow the code below to remove any existing session widgets
    }
    else {
        ui.labelCoreVersion->setText(coreInfo["quasselVersion"].toString());
        // "BuildDate" for compatibility
        if (coreInfo["quasselBuildDate"].toString().isEmpty()) {
            ui.labelCoreVersionDate->setText(QString("<i>%1</i>").arg(tr("Unknown date")));
        }
        else {
            ui.labelCoreVersionDate->setText(
                tryFormatUnixEpoch(coreInfo["quasselBuildDate"].toString(), Qt::DateFormat::DefaultLocaleShortDate));
        }
        ui.labelClientCount->setNum(coreInfo["sessionConnectedClients"].toInt());
    }

    auto coreSessionSupported = false;
    auto ids = _widgets.keys();
    for (const auto& peerData : coreInfo["sessionConnectedClientData"].toList()) {
        coreSessionSupported = true;

        auto peerMap = peerData.toMap();
        int peerId = peerMap["id"].toInt();

        ids.removeAll(peerId);

        bool isNew = false;
        CoreSessionWidget* coreSessionWidget = _widgets[peerId];
        if (coreSessionWidget == nullptr) {
            coreSessionWidget = new CoreSessionWidget(ui.coreSessionScrollContainer);
            isNew = true;
        }
        coreSessionWidget->setData(peerMap);
        if (isNew) {
            _widgets[peerId] = coreSessionWidget;
            // Add this to the end of the session list, but before the default layout stretch item.
            // The layout stretch item should never be removed, so count should always be >= 1.
            ui.coreSessionContainer->insertWidget(ui.coreSessionContainer->count() - 1, coreSessionWidget, 0, Qt::AlignTop);
            connect(coreSessionWidget, &CoreSessionWidget::disconnectClicked, this, &CoreInfoDlg::disconnectClicked);
        }
    }

    for (const auto& key : ids) {
        delete _widgets[key];
        _widgets.remove(key);
    }

    ui.coreSessionScrollArea->setVisible(coreSessionSupported);

    // Hide the information bar when core sessions are supported or when disconnected
    ui.coreUnsupportedWidget->setVisible(!(coreSessionSupported || Client::isConnected() == false));

    // Update uptime for immediate display, don't wait for the timer
    updateUptime();
}

void CoreInfoDlg::updateUptime()
{
    CoreInfo* coreInfo = Client::coreInfo();

    if (!Client::isConnected()) {
        // Not connected, don't bother trying to calculate the uptime
        ui.labelUptime->setText(tr("Not available"));
    }
    else if (coreInfo->coreData().isEmpty()) {
        // Core info is entirely empty despite being connected.  Something's probably wrong.
        ui.labelUptime->setText(tr("Unknown"));
    }
    else {
        // Connected, format the uptime for display
        QDateTime startTime = coreInfo->at("startTime").toDateTime();

        int64_t uptime = startTime.secsTo(QDateTime::currentDateTime().toUTC());
        int64_t updays = uptime / 86400;
        uptime %= 86400;
        int64_t uphours = uptime / 3600;
        uptime %= 3600;
        int64_t upmins = uptime / 60;
        uptime %= 60;

        QString uptimeText = tr("%n Day(s)", "", updays)
                             + tr(" %1:%2:%3 (since %4)")
                                   .arg(uphours, 2, 10, QChar('0'))
                                   .arg(upmins, 2, 10, QChar('0'))
                                   .arg(uptime, 2, 10, QChar('0'))
                                   .arg(startTime.toLocalTime().toString(Qt::DefaultLocaleShortDate));
        ui.labelUptime->setText(uptimeText);
    }
}

void CoreInfoDlg::disconnectClicked(int peerId)
{
    Client::kickClient(peerId);
}

void CoreInfoDlg::on_coreUnsupportedDetails_clicked()
{
    QMessageBox::warning(this,
                         tr("Active sessions unsupported"),
                         QString("<p><b>%1</b></p></br><p>%2</p>")
                             .arg(tr("Your Quassel core is too old to show active sessions"),
                                  tr("You need a Quassel core v0.13.0 or newer to view and "
                                     "disconnect other connected clients.")));
}
