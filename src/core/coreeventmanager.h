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

#ifndef COREEVENTMANAGER_H
#define COREEVENTMANAGER_H

#include "corenetwork.h"
#include "coresession.h"
#include "eventmanager.h"

class CoreSession;

class CoreEventManager : public EventManager
{
    Q_OBJECT

public:
    CoreEventManager(CoreSession *session)
        : EventManager(session)
        , _coreSession(session)
    {}

protected:
    inline Network *networkById(NetworkId id) const { return _coreSession->network(id); }

private:
    CoreSession *_coreSession;
};


#endif
