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

#include "coreinfodlg.h"

#include <QDateTime>

#include "client.h"
#include "signalproxy.h"

CoreInfoDlg::CoreInfoDlg(QWidget *parent)
    : QDialog(parent),
    _coreInfo(this)
{
    ui.setupUi(this);
    connect(&_coreInfo, SIGNAL(initDone()), this, SLOT(coreInfoAvailable()));
    Client::signalProxy()->synchronize(&_coreInfo);
}


void CoreInfoDlg::coreInfoAvailable()
{
    ui.labelCoreVersion->setText(_coreInfo["quasselVersion"].toString());
    ui.labelCoreBuildDate->setText(_coreInfo["quasselBuildDate"].toString());
    ui.labelClientCount->setNum(_coreInfo["sessionConnectedClients"].toInt());
    updateUptime();
    startTimer(1000);
}


void CoreInfoDlg::updateUptime()
{
    QDateTime startTime = _coreInfo["startTime"].toDateTime();

    int uptime = startTime.secsTo(QDateTime::currentDateTime().toUTC());
    int updays = uptime / 86400; uptime %= 86400;
    int uphours = uptime / 3600; uptime %= 3600;
    int upmins = uptime / 60; uptime %= 60;

    QString uptimeText = tr("%n Day(s)", "", updays)
                         + tr(" %1:%2:%3 (since %4)").arg(uphours, 2, 10, QChar('0')).arg(upmins, 2, 10, QChar('0')).arg(uptime, 2, 10, QChar('0')).arg(startTime.toLocalTime().toString(Qt::TextDate));
    ui.labelUptime->setText(uptimeText);
}
