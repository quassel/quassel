/***************************************************************************
 *   Copyright (C) 2005-2022 by the Quassel Project                        *
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

#include "corebufferviewmanager.h"

#include "core.h"
#include "corebufferviewconfig.h"
#include "coresession.h"

CoreBufferViewManager::CoreBufferViewManager(SignalProxy* proxy, CoreSession* parent)
    : BufferViewManager(proxy, parent)
    , _coreSession(parent)
{
    QVariantMap views = Core::getUserSetting(_coreSession->user(), "BufferViews").toMap();
    QVariantMap::iterator iter = views.begin();
    QVariantMap::iterator iterEnd = views.end();
    CoreBufferViewConfig* config = nullptr;
    while (iter != iterEnd) {
        config = new CoreBufferViewConfig(iter.key().toInt(), iter.value().toMap(), this);
        addBufferViewConfig(config);
        ++iter;
    }
}

void CoreBufferViewManager::saveBufferViews()
{
    QVariantMap views;

    BufferViewConfigHash::const_iterator iter = bufferViewConfigHash().constBegin();
    BufferViewConfigHash::const_iterator iterEnd = bufferViewConfigHash().constEnd();
    while (iter != iterEnd) {
        views[QString::number((*iter)->bufferViewId())] = (*iter)->toVariantMap();
        ++iter;
    }

    Core::setUserSetting(_coreSession->user(), "BufferViews", views);
}

void CoreBufferViewManager::requestCreateBufferView(const QVariantMap& properties)
{
    QString bufferViewName = properties["bufferViewName"].toString();
    int maxId = -1;
    BufferViewConfigHash::const_iterator iter = bufferViewConfigHash().constBegin();
    BufferViewConfigHash::const_iterator iterEnd = bufferViewConfigHash().constEnd();
    while (iter != iterEnd) {
        if ((*iter)->bufferViewName() == bufferViewName)
            return;

        if ((*iter)->bufferViewId() > maxId)
            maxId = (*iter)->bufferViewId();

        ++iter;
    }
    maxId++;

    addBufferViewConfig(new CoreBufferViewConfig(maxId, properties, this));
}

void CoreBufferViewManager::requestCreateBufferViews(const QVariantList& properties)
{
    QVariantList::const_iterator iter = properties.constBegin();
    QVariantList::const_iterator iterEnd = properties.constEnd();
    while (iter != iterEnd) {
        requestCreateBufferView((*iter).toMap());
        ++iter;
    }
}

void CoreBufferViewManager::requestDeleteBufferView(int bufferViewId)
{
    deleteBufferViewConfig(bufferViewId);
}

void CoreBufferViewManager::requestDeleteBufferViews(const QVariantList& bufferViews)
{
    for (const QVariant& bufferView : bufferViews) {
        deleteBufferViewConfig(bufferView.toInt());
    }
}
