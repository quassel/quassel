/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
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

#include <QDateTime>

#include "client.h"
#include "coresessionwidget.h"


CoreSessionWidget::CoreSessionWidget(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    connect(ui.disconnectButton, SIGNAL(released()), this, SLOT(disconnectClicked()));
}

void CoreSessionWidget::setData(QMap<QString, QVariant> map)
{
    ui.sessionGroup->setTitle(map["remoteAddress"].toString());
    ui.labelLocation->setText(map["location"].toString());
    ui.labelClient->setText(map["clientVersion"].toString());
    ui.labelVersionDate->setText(map["clientVersionDate"].toString());
    ui.labelUptime->setText(map["connectedSince"].toDateTime().toLocalTime().toString(Qt::DateFormat::SystemLocaleShortDate));
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
    } else {
        // Either core or client doesn't support it, disable the option
        ui.disconnectButton->setEnabled(false);
        if (!Client::isCoreFeatureEnabled(Quassel::Feature::RemoteDisconnect)) {
            // Until RemoteDisconnect was implemented, the Quassel core didn't forward client
            // features.  A client might support features and we'll never hear about it.
            // This check shouldn't be necessary for later features if the core supports at least
            // RemoteDisconnect.

            // Core doesn't support this feature (we don't know about the client)
            ui.disconnectButton->setToolTip(
                        QString("<p>%1</p><p><b>%2</b><br/>%3</p>").arg(
                            tr("End the client's session, disconnecting it"),
                            tr("Your Quassel core does not support this feature"),
                            tr("You need a Quassel core v0.13.0 or newer in order to end connected "
                               "sessions.")));
        } else {
            // Client doesn't support this feature
            ui.disconnectButton->setToolTip(
                        QString("<p>%1</p><p><b>%2</b></p>").arg(
                            tr("End the client's session, disconnecting it"),
                            tr("This client does not support being remotely disconnected")));
        }
    }

    bool success = false;
    _peerId = map["id"].toInt(&success);
    if (!success) _peerId = -1;
}

void CoreSessionWidget::disconnectClicked()
{
    // Don't allow the End Session button to be spammed; Quassel's protocol isn't lossy and it
    // should reach the destination eventually...
    ui.disconnectButton->setEnabled(false);
    ui.disconnectButton->setText(tr("Ending session..."));

    emit disconnectClicked(_peerId);
}
