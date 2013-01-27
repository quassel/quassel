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

#ifndef COREBUFFERVIEWMANAGER_H
#define COREBUFFERVIEWMANAGER_H

#include "bufferviewmanager.h"

class CoreSession;

class CoreBufferViewManager : public BufferViewManager
{
    SYNCABLE_OBJECT
        Q_OBJECT

public:
    CoreBufferViewManager(SignalProxy *proxy, CoreSession *parent);

    inline virtual const QMetaObject *syncMetaObject() const { return &BufferViewManager::staticMetaObject; }

public slots:
    virtual void requestCreateBufferView(const QVariantMap &properties);
    virtual void requestCreateBufferViews(const QVariantList &properties);
    virtual void requestDeleteBufferView(int bufferViewId);
    virtual void requestDeleteBufferViews(const QVariantList &bufferViews);

    void saveBufferViews();

private:
    CoreSession *_coreSession;
};


#endif // COREBUFFERVIEWMANAGER_H
