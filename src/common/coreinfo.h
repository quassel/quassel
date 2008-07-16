/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel Project                          *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef COREINFO_H
#define COREINFO_H

#include "syncableobject.h"

/*
 * gather various informations about the core.
 */

class CoreInfo : public SyncableObject {
  Q_OBJECT

  Q_PROPERTY(QVariantMap coreData READ coreData WRITE setCoreData STORED false)

public:
  CoreInfo(QObject *parent = 0) : SyncableObject(parent) {}

public slots:
  virtual inline QVariantMap coreData() const { return QVariantMap(); }
  virtual inline void setCoreData(const QVariantMap &) {}
};

#endif //COREINFO_H
