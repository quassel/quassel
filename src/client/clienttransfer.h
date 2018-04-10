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

#ifndef CLIENTTRANSFER_H
#define CLIENTTRANSFER_H

#include <QUuid>

#include "transfer.h"

class QFile;

class ClientTransfer : public Transfer
{
    Q_OBJECT
    SYNCABLE_OBJECT

public:
    ClientTransfer(const QUuid &uuid, QObject *parent = 0);

    QString savePath() const;

public slots:
    // called on the client side
    void accept(const QString &savePath) const;
    void reject() const;

private slots:
    void dataReceived(PeerPtr peer, const QByteArray &data);
    void onStateChanged(State state);

private:
    virtual void cleanUp();

    mutable QString _savePath;

    QFile *_file;
};

#endif
