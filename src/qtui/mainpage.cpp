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

#include "mainpage.h"

#include <QImage>
#include <QLabel>
#include <QLayout>
#include <QPainter>
#include <QPushButton>

#include "client.h"
#include "coreconnectdlg.h"
#include "icon.h"

MainPage::MainPage(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    QLabel* label = new QLabel(this);
    label->setPixmap(QPixmap(":/pics/quassel-logo.png"));
    layout->addWidget(label);

    if (Quassel::runMode() != Quassel::Monolithic) {
        _connectButton = new QPushButton(icon::get("network-connect"), tr("Connect to Core..."));
        _connectButton->setEnabled(Client::coreConnection()->state() == CoreConnection::Disconnected);

        connect(Client::coreConnection(), &CoreConnection::stateChanged, this, &MainPage::coreConnectionStateChanged);
        connect(_connectButton, &QAbstractButton::clicked, this, &MainPage::showCoreConnectionDlg);
        layout->addWidget(_connectButton);
    }
}

void MainPage::showCoreConnectionDlg()
{
    CoreConnectDlg dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        AccountId accId = dlg.selectedAccount();
        if (accId.isValid())
            Client::coreConnection()->connectToCore(accId);
    }
}

void MainPage::coreConnectionStateChanged()
{
    if (Client::coreConnection()->state() == CoreConnection::Disconnected) {
        _connectButton->setEnabled(true);
    }
    else {
        _connectButton->setDisabled(true);
    }
}
