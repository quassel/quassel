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

#include "coreconnectionstatuswidget.h"

#include "client.h"
#include "iconloader.h"
#include "signalproxy.h"

CoreConnectionStatusWidget::CoreConnectionStatusWidget(CoreConnection *connection, QWidget *parent)
    : QWidget(parent),
    _coreConnection(connection)
{
    ui.setupUi(this);
    ui.lagLabel->hide();
    ui.sslLabel->hide();
    update();

    connect(coreConnection(), SIGNAL(progressTextChanged(QString)), ui.messageLabel, SLOT(setText(QString)));
    connect(coreConnection(), SIGNAL(progressValueChanged(int)), ui.progressBar, SLOT(setValue(int)));
    connect(coreConnection(), SIGNAL(progressRangeChanged(int, int)), ui.progressBar, SLOT(setRange(int, int)));
    connect(coreConnection(), SIGNAL(progressRangeChanged(int, int)), this, SLOT(progressRangeChanged(int, int)));

    connect(coreConnection(), SIGNAL(stateChanged(CoreConnection::ConnectionState)), SLOT(connectionStateChanged(CoreConnection::ConnectionState)));
    connect(coreConnection(), SIGNAL(connectionError(QString)), ui.messageLabel, SLOT(setText(QString)));
    connect(coreConnection(), SIGNAL(lagUpdated(int)), SLOT(updateLag(int)));
}


void CoreConnectionStatusWidget::update()
{
    CoreConnection *conn = coreConnection();
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
            ui.sslLabel->setPixmap(SmallIcon("security-high"));
            ui.sslLabel->setToolTip(tr("The connection to your core is encrypted with SSL."));
        }
        else {
            ui.sslLabel->setPixmap(SmallIcon("security-low"));
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
