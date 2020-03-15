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

#include "coresessionwidget.h"

#include <QDateTime>

#include "client.h"
#include "util.h"

CoreSessionWidget::CoreSessionWidget(QWidget* parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    connect(ui.disconnectButton, &QPushButton::released, this, &CoreSessionWidget::onDisconnectClicked);
}

void CoreSessionWidget::setData(QMap<QString, QVariant> map)
{
    ui.sessionGroup->setTitle(map["remoteAddress"].toString());
    ui.labelLocation->setText(map["location"].toString());
    ui.labelClient->setText(map["clientVersion"].toString());
    if (map["clientVersionDate"].toString().isEmpty()) {
        ui.labelVersionDate->setText(QString("<i>%1</i>").arg(tr("Unknown date")));
    }
    else {
        ui.labelVersionDate->setText(tryFormatUnixEpoch(map["clientVersionDate"].toString(), Qt::DateFormat::DefaultLocaleShortDate));
    }
    ui.labelUptime->setText(map["connectedSince"].toDateTime().toLocalTime().toString(Qt::DateFormat::DefaultLocaleShortDate));
    if (map["location"].toString().isEmpty()) {
        ui.labelLocation->hide();
        ui.labelLocationTitle->hide();
    }
    ui.labelSecure->setText(map["secure"].toBool() ? tr("Yes") : tr("No"));

    auto features = Quassel::Features{map["featureList"].toStringList(), static_cast<Quassel::LegacyFeatures>(map["features"].toUInt())};
    if (features.isEnabled(Quassel::Feature::RemoteDisconnect)) {
        // Both client and core support it, enable the button
        ui.disconnectButton->setEnabled(true);
        ui.disconnectButton->setToolTip(tr("End the client's session, disconnecting it"));
    }
    else {
        // For any active sessions to be displayed, the core must support this feature.  We can
        // assume the client doesn't support being remotely disconnected.
        //
        // (During the development of 0.13, there was a period of time where active sessions existed
        //  but did not provide the disconnect option.  We can overlook this.)

        // Either core or client doesn't support it, disable the option
        ui.disconnectButton->setEnabled(false);
        // Assuming the client lacks support, set the tooltip accordingly
        ui.disconnectButton->setToolTip(
            QString("<p>%1</p><p><b>%2</b></p>")
                .arg(tr("End the client's session, disconnecting it"), tr("This client does not support being remotely disconnected")));
    }

    bool success = false;
    _peerId = map["id"].toInt(&success);
    if (!success)
        _peerId = -1;
}

void CoreSessionWidget::onDisconnectClicked()
{
    // Don't allow the End Session button to be spammed; Quassel's protocol isn't lossy and it
    // should reach the destination eventually...
    ui.disconnectButton->setEnabled(false);
    ui.disconnectButton->setText(tr("Ending session..."));

    emit disconnectClicked(_peerId);
}
