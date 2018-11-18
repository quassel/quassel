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

#pragma once

#include <QPointer>
#include <QThread>

#include "qtuiapplication.h"

class Core;
class InternalPeer;

class MonolithicApplication : public QtUiApplication
{
    Q_OBJECT

public:
    MonolithicApplication(int &, char **);

    void init() override;

protected:
    Quassel::QuitHandler quitHandler() override;

signals:
    void connectInternalPeer(QPointer<InternalPeer> peer);

private slots:
    void onConnectionRequest(QPointer<InternalPeer> peer);
    void onClientDestroyed();
    void onCoreShutdown();

private:
    void startInternalCore();

private:
    QPointer<Core> _core;
    QThread _coreThread;
};
