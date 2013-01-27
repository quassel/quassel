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

#ifndef CLIENTCOREINFO_H
#define CLIENTCOREINFO_H

#include "coreinfo.h"

/*
 * Yes this name is somewhat stupid... but it fits the general naming scheme
 * which is prefixing client specific sync objects with "Client"... ;)
 */
class ClientCoreInfo : public CoreInfo
{
    Q_OBJECT
        SYNCABLE_OBJECT

public:
    ClientCoreInfo(QObject *parent = 0) : CoreInfo(parent) {}

    inline virtual const QMetaObject *syncMetaObject() const { return &CoreInfo::staticMetaObject; }

    inline QVariant &operator[](const QString &key) { return _coreData[key]; }

public slots:
    inline virtual void setCoreData(const QVariantMap &data) { _coreData = data; }

private:
    QVariantMap _coreData;
};


#endif //CLIENTCOREINFO_H
