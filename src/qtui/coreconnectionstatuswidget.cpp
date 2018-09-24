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

#include "coreconnectionstatuswidget.h"

#include "client.h"
#include "icon.h"
#include "signalproxy.h"

CoreConnectionStatusWidget::CoreConnectionStatusWidget(CoreConnection* connection, QWidget* parent)
    : QWidget(parent)
    , _coreConnection(connection)
{
    ui.setupUi(this);
    ui.lagLabel->hide();
    ui.sslLabel->hide();
    update();

    connect(coreConnection(), &CoreConnection::progressTextChanged, ui.messageLabel, &QLabel::setText);
    connect(coreConnection(), &CoreConnection::progressValueChanged, ui.progressBar, &QProgressBar::setValue);
    connect(coreConnection(), &CoreConnection::progressRangeChanged, ui.progressBar, &QProgressBar::setRange);
    connect(coreConnection(), &CoreConnection::progressRangeChanged, this, &CoreConnectionStatusWidget::progressRangeChanged);

    connect(coreConnection(), &CoreConnection::stateChanged, this, &CoreConnectionStatusWidget::connectionStateChanged);
    connect(coreConnection(), &CoreConnection::connectionError, ui.messageLabel, &QLabel::setText);
    connect(coreConnection(), &CoreConnection::lagUpdated, this, &CoreConnectionStatusWidget::updateLag);
}

void CoreConnectionStatusWidget::update()
{
    CoreConnection* conn = coreConnection();
    if (conn->progressMaximum() >= 0) {
        ui.progressBar->setMinimum(conn->progressMinimum());
        ui.progressBar->setMaximum(conn->progressMaximum());
        ui.progressBar->setValue(conn->progressValue());
        ui.progressBar->show();
    }
    else
        ui.progressBar->hide();

    ui.messageLabel->setText(conn->progressText());
}

void CoreConnectionStatusWidget::updateLag(int msecs)
{
    if (msecs >= 0) {
        QString unit = msecs >= 100 ? tr("s", "seconds") : tr("ms", "milliseconds");
        ui.lagLabel->setText(tr("(Lag: %1 %2)").arg(msecs >= 100 ? msecs / 1000. : msecs, 0, 'f', (int)(msecs >= 100)).arg(unit));
        if (!ui.lagLabel->isVisible())
            ui.lagLabel->show();
    }
    else {
        if (ui.lagLabel->isVisible())
            ui.lagLabel->hide();
    }
}

void CoreConnectionStatusWidget::connectionStateChanged(CoreConnection::ConnectionState state)
{
    if (state >= CoreConnection::Connected) {
        if (coreConnection()->isEncrypted()) {
            ui.sslLabel->setPixmap(icon::get("security-high").pixmap(16));
            ui.sslLabel->setToolTip(tr("The connection to your core is encrypted with SSL."));
        }
        else {
            ui.sslLabel->setPixmap(icon::get("security-low").pixmap(16));
            ui.sslLabel->setToolTip(tr("The connection to your core is not encrypted."));
        }
        ui.sslLabel->show();
    }
    else
        ui.sslLabel->hide();
}

void CoreConnectionStatusWidget::progressRangeChanged(int min, int max)
{
    Q_UNUSED(min)
    ui.progressBar->setVisible(max >= 0);
}
