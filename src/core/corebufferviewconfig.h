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

#include "bufferviewconfig.h"

#ifndef COREBUFFERVIEWCONFIG_H
#define COREBUFFERVIEWCONFIG_H

class CoreBufferViewConfig : public BufferViewConfig
{
    SYNCABLE_OBJECT
        Q_OBJECT

public:
    CoreBufferViewConfig(int bufferViewId, QObject *parent = 0);
    CoreBufferViewConfig(int bufferViewId, const QVariantMap &properties, QObject *parent = 0);

    inline virtual const QMetaObject *syncMetaObject() const { return &BufferViewConfig::staticMetaObject; }

public slots:
    virtual inline void requestSetBufferViewName(const QString &bufferViewName) { setBufferViewName(bufferViewName); }
    virtual inline void requestRemoveBuffer(const BufferId &bufferId) { removeBuffer(bufferId); }
    virtual inline void requestRemoveBufferPermanently(const BufferId &bufferId) { removeBufferPermanently(bufferId); }
    virtual inline void requestAddBuffer(const BufferId &bufferId, int pos) { addBuffer(bufferId, pos); }
    virtual inline void requestMoveBuffer(const BufferId &bufferId, int pos) { moveBuffer(bufferId, pos); }
};


#endif // COREBUFFERVIEWCONFIG_H
