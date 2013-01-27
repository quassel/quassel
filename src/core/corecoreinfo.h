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

#ifndef CORECOREINFO_H
#define CORECOREINFO_H

#include "coreinfo.h"

class CoreSession;

/*
 * Yes this name is somewhat stupid... but it fits the general naming scheme
 * which is prefixing core specific sync objects with "Core"... ;)
 */
class CoreCoreInfo : public CoreInfo
{
    SYNCABLE_OBJECT
        Q_OBJECT

public:
    CoreCoreInfo(CoreSession *parent);

    inline virtual const QMetaObject *syncMetaObject() const { return &CoreInfo::staticMetaObject; }

public slots:
    virtual QVariantMap coreData() const;

private:
    CoreSession *_coreSession;
};


#endif //CORECOREINFO_H
