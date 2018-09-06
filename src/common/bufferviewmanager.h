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

#include "common-export.h"

#include "syncableobject.h"

#include <QList>
#include <QHash>

class BufferViewConfig;
class SignalProxy;

class COMMON_EXPORT BufferViewManager : public SyncableObject
{
    Q_OBJECT
    SYNCABLE_OBJECT

public:
    BufferViewManager(SignalProxy *proxy, QObject *parent = nullptr);

    inline QList<BufferViewConfig *> bufferViewConfigs() const { return _bufferViewConfigs.values(); }
    BufferViewConfig *bufferViewConfig(int bufferViewId) const;

public slots:
    QVariantList initBufferViewIds() const;
    void initSetBufferViewIds(const QVariantList bufferViewIds);

    void addBufferViewConfig(int bufferViewConfigId);
    void deleteBufferViewConfig(int bufferViewConfigId);

    virtual inline void requestCreateBufferView(const QVariantMap &properties) { REQUEST(ARG(properties)) }
    virtual inline void requestCreateBufferViews(const QVariantList &properties) { REQUEST(ARG(properties)) }
    virtual inline void requestDeleteBufferView(int bufferViewId) { REQUEST(ARG(bufferViewId)) }
    virtual inline void requestDeleteBufferViews(const QVariantList &bufferViews) { REQUEST(ARG(bufferViews)) }

signals:
    void bufferViewConfigAdded(int bufferViewConfigId);
    void bufferViewConfigDeleted(int bufferViewConfigId);
//   void createBufferViewRequested(const QVariantMap &properties);
//   void createBufferViewsRequested(const QVariantList &properties);
//   void deleteBufferViewRequested(int bufferViewId);
//   void deleteBufferViewsRequested(const QVariantList &bufferViews);

protected:
    using  BufferViewConfigHash = QHash<int, BufferViewConfig *>;
    inline const BufferViewConfigHash &bufferViewConfigHash() { return _bufferViewConfigs; }
    virtual BufferViewConfig *bufferViewConfigFactory(int bufferViewConfigId);

    void addBufferViewConfig(BufferViewConfig *config);

private:
    BufferViewConfigHash _bufferViewConfigs;
    SignalProxy *_proxy;
};
