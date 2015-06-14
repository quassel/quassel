/***************************************************************************
 *   Copyright (C) 2005-2016 by the Quassel Project                        *
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

#ifndef BUFFERVIEWCONFIG_H
#define BUFFERVIEWCONFIG_H

#include "syncableobject.h"

#include "types.h"

class BufferViewConfig : public SyncableObject
{
    SYNCABLE_OBJECT
    Q_OBJECT

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

public :
        BufferViewConfig(int bufferViewId, QObject *parent = 0);
    BufferViewConfig(int bufferViewId, const QVariantMap &properties, QObject *parent = 0);

    inline virtual const QMetaObject *syncMetaObject() const { return &staticMetaObject; }

public slots:
    inline int bufferViewId() const { return _bufferViewId; }

    inline const QString &bufferViewName() const { return _bufferViewName; }
    void setBufferViewName(const QString &bufferViewName);

    inline const NetworkId &networkId() const { return _networkId; }
    void setNetworkId(const NetworkId &networkId);

    inline bool addNewBuffersAutomatically() const { return _addNewBuffersAutomatically; }
    void setAddNewBuffersAutomatically(bool addNewBuffersAutomatically);

    inline bool sortAlphabetically() const { return _sortAlphabetically; }
    void setSortAlphabetically(bool sortAlphabetically);

    inline bool disableDecoration() const { return _disableDecoration; }
    void setDisableDecoration(bool disableDecoration);

    inline int allowedBufferTypes() const { return _allowedBufferTypes; }
    void setAllowedBufferTypes(int bufferTypes);

    inline int minimumActivity() const { return _minimumActivity; }
    void setMinimumActivity(int activity);

    inline bool hideInactiveBuffers() const { return _hideInactiveBuffers; }
    void setHideInactiveBuffers(bool hideInactiveBuffers);

    inline bool hideInactiveNetworks() const { return _hideInactiveNetworks; }
    void setHideInactiveNetworks(bool hideInactiveNetworks);

    inline bool showSearch() const { return _showSearch; }
    void setShowSearch(bool showSearch);

    virtual inline void requestSetBufferViewName(const QString &bufferViewName) { REQUEST(ARG(bufferViewName)) }

    const QList<BufferId> &bufferList() const { return _buffers; }
    const QSet<BufferId> &removedBuffers() const { return _removedBuffers; }
    const QSet<BufferId> &temporarilyRemovedBuffers() const { return _temporarilyRemovedBuffers; }

    QVariantList initBufferList() const;
    void initSetBufferList(const QVariantList &buffers);
    void initSetBufferList(const QList<BufferId> &buffers);

    QVariantList initRemovedBuffers() const;
    void initSetRemovedBuffers(const QVariantList &buffers);

    QVariantList initTemporarilyRemovedBuffers() const;
    void initSetTemporarilyRemovedBuffers(const QVariantList &buffers);

    void addBuffer(const BufferId &bufferId, int pos);
    virtual inline void requestAddBuffer(const BufferId &bufferId, int pos) { REQUEST(ARG(bufferId), ARG(pos)) }
    void moveBuffer(const BufferId &bufferId, int pos);
    virtual inline void requestMoveBuffer(const BufferId &bufferId, int pos) { REQUEST(ARG(bufferId), ARG(pos)) }
    void removeBuffer(const BufferId &bufferId);
    virtual inline void requestRemoveBuffer(const BufferId &bufferId) { REQUEST(ARG(bufferId)) }
    void removeBufferPermanently(const BufferId &bufferId);
    virtual inline void requestRemoveBufferPermanently(const BufferId &bufferId) { REQUEST(ARG(bufferId)) }

signals:
    void bufferViewNameSet(const QString &bufferViewName); // invalidate
    void configChanged();
    void networkIdSet(const NetworkId &networkId);
//   void addNewBuffersAutomaticallySet(bool addNewBuffersAutomatically); // invalidate
//   void sortAlphabeticallySet(bool sortAlphabetically); // invalidate
//   //  void disableDecorationSet(bool disableDecoration); // invalidate
//   void allowedBufferTypesSet(int allowedBufferTypes); // invalidate
//   void minimumActivitySet(int activity); // invalidate
//   void hideInactiveBuffersSet(bool hideInactiveBuffers); // invalidate
    void bufferListSet(); // invalidate

    void bufferAdded(const BufferId &bufferId, int pos);
//   void addBufferRequested(const BufferId &bufferId, int pos);
    void bufferMoved(const BufferId &bufferId, int pos);
//   void moveBufferRequested(const BufferId &bufferId, int pos);
    void bufferRemoved(const BufferId &bufferId);
    void bufferPermanentlyRemoved(const BufferId &bufferId);
//   void removeBufferRequested(const BufferId &bufferId);
//   void removeBufferPermanentlyRequested(const BufferId &bufferId);

//   void setBufferViewNameRequested(const QString &bufferViewName);

private:
    int _bufferViewId;
    QString _bufferViewName;
    NetworkId _networkId;
    bool _addNewBuffersAutomatically;
    bool _sortAlphabetically;
    bool _hideInactiveBuffers;
    bool _hideInactiveNetworks;
    bool _disableDecoration;
    int _allowedBufferTypes;
    int _minimumActivity;
    bool _showSearch;
    QList<BufferId> _buffers;
    QSet<BufferId> _removedBuffers;
    QSet<BufferId> _temporarilyRemovedBuffers;
};


#endif // BUFFERVIEWCONFIG_H
