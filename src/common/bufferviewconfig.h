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

#include "bufferinfo.h"
#include "syncableobject.h"
#include "types.h"

class COMMON_EXPORT BufferViewConfig : public SyncableObject
{
    Q_OBJECT
    SYNCABLE_OBJECT

    Q_PROPERTY(QString bufferViewName READ bufferViewName WRITE setBufferViewName)
    Q_PROPERTY(NetworkId networkId READ networkId WRITE setNetworkId)
    Q_PROPERTY(bool addNewBuffersAutomatically READ addNewBuffersAutomatically WRITE setAddNewBuffersAutomatically)
    Q_PROPERTY(bool sortAlphabetically READ sortAlphabetically WRITE setSortAlphabetically)
    Q_PROPERTY(bool hideInactiveBuffers READ hideInactiveBuffers WRITE setHideInactiveBuffers)
    Q_PROPERTY(bool hideInactiveNetworks READ hideInactiveNetworks WRITE setHideInactiveNetworks)
    Q_PROPERTY(bool disableDecoration READ disableDecoration WRITE setDisableDecoration)
    Q_PROPERTY(int allowedBufferTypes READ allowedBufferTypes WRITE setAllowedBufferTypes)
    Q_PROPERTY(int minimumActivity READ minimumActivity WRITE setMinimumActivity)
    Q_PROPERTY(bool showSearch READ showSearch WRITE setShowSearch)

public:
    BufferViewConfig(int bufferViewId, QObject* parent = nullptr);
    BufferViewConfig(int bufferViewId, const QVariantMap& properties, QObject* parent = nullptr);

public:
    int bufferViewId() const;
    QString bufferViewName() const;
    NetworkId networkId() const;
    bool addNewBuffersAutomatically() const;
    bool sortAlphabetically() const;
    bool disableDecoration() const;
    int allowedBufferTypes() const;
    int minimumActivity() const;
    bool hideInactiveBuffers() const;
    bool hideInactiveNetworks() const;
    bool showSearch() const;

    QList<BufferId> bufferList() const;
    QSet<BufferId> removedBuffers() const;
    QSet<BufferId> temporarilyRemovedBuffers() const;

public slots:
    QVariantList initBufferList() const;
    void initSetBufferList(const QVariantList& buffers);

    QVariantList initRemovedBuffers() const;
    void initSetRemovedBuffers(const QVariantList& buffers);

    QVariantList initTemporarilyRemovedBuffers() const;
    void initSetTemporarilyRemovedBuffers(const QVariantList& buffers);

    void setBufferViewName(const QString& bufferViewName);
    void setNetworkId(const NetworkId& networkId);
    void setAddNewBuffersAutomatically(bool addNewBuffersAutomatically);
    void setSortAlphabetically(bool sortAlphabetically);
    void setDisableDecoration(bool disableDecoration);
    void setAllowedBufferTypes(int bufferTypes);
    void setMinimumActivity(int activity);
    void setHideInactiveBuffers(bool hideInactiveBuffers);
    void setHideInactiveNetworks(bool hideInactiveNetworks);
    void setShowSearch(bool showSearch);

    void setBufferList(const QList<BufferId>& buffers);

    void addBuffer(const BufferId& bufferId, int pos);
    void moveBuffer(const BufferId& bufferId, int pos);
    void removeBuffer(const BufferId& bufferId);
    void removeBufferPermanently(const BufferId& bufferId);

    virtual void requestSetBufferViewName(const QString& bufferViewName);
    virtual void requestAddBuffer(const BufferId& bufferId, int pos);
    virtual void requestMoveBuffer(const BufferId& bufferId, int pos);
    virtual void requestRemoveBuffer(const BufferId& bufferId);
    virtual void requestRemoveBufferPermanently(const BufferId& bufferId);

signals:
    void configChanged();

    void bufferViewNameSet(const QString& bufferViewName);
    void networkIdSet(const NetworkId& networkId);
    void bufferListSet();

    void bufferAdded(const BufferId& bufferId, int pos);
    void bufferMoved(const BufferId& bufferId, int pos);
    void bufferRemoved(const BufferId& bufferId);
    void bufferPermanentlyRemoved(const BufferId& bufferId);

private:
    int _bufferViewId = 0;         ///< ID of the associated BufferView
    QString _bufferViewName = {};  ///< Display name of the associated BufferView
    NetworkId _networkId = {};     ///< Network ID this buffer belongs to

    bool _addNewBuffersAutomatically = true;  ///< Automatically add new buffers when created
    bool _sortAlphabetically = true;          ///< Sort buffers alphabetically
    bool _hideInactiveBuffers = false;        ///< Hide buffers without activity
    bool _hideInactiveNetworks = false;       ///< Hide networks without activity
    bool _disableDecoration = false;          ///< Disable buffer decoration (not fully implemented)
    /// Buffer types allowed within this view
    int _allowedBufferTypes = (BufferInfo::StatusBuffer | BufferInfo::ChannelBuffer | BufferInfo::QueryBuffer | BufferInfo::GroupBuffer);
    int _minimumActivity = 0;  ///< Minimum activity for a buffer to show
    bool _showSearch = false;  ///< Persistently show the buffer search UI

    QList<BufferId> _buffers;
    QSet<BufferId> _removedBuffers;
    QSet<BufferId> _temporarilyRemovedBuffers;
};
