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

    auto features = Quassel::Features(map["features"].toInt());
    ui.disconnectButton->setVisible(features.testFlag(Quassel::Feature::RemoteDisconnect));

    bool success = false;
    _peerId = map["id"].toInt(&success);
    if (!success) _peerId = -1;
}

void CoreSessionWidget::disconnectClicked()
{
    emit disconnectClicked(_peerId);
}
