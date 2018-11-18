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

#ifndef CORECONNECTIONSTATUSWIDGET_H
#define CORECONNECTIONSTATUSWIDGET_H

#include <QWidget>

#include "coreconnection.h"

#include "ui_coreconnectionstatuswidget.h"

class CoreConnectionStatusWidget : public QWidget
{
    Q_OBJECT

public:
    CoreConnectionStatusWidget(CoreConnection* connection, QWidget* parent = nullptr);

    inline CoreConnection* coreConnection() const { return _coreConnection; }

public slots:
    void update();
    void updateLag(int msecs);

private slots:
    void connectionStateChanged(CoreConnection::ConnectionState);
    void progressRangeChanged(int min, int max);

private:
    Ui::CoreConnectionStatusWidget ui;

    CoreConnection* _coreConnection;
};

#endif  // CORECONNECTIONSTATUSWIDGET_H
