/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
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

#ifndef COREIGNORELISTMANAGER_H
#define COREIGNORELISTMANAGER_H

#include "ignorelistmanager.h"

class CoreSession;

class CoreIgnoreListManager : public IgnoreListManager {
  SYNCABLE_OBJECT
  Q_OBJECT

public:
  explicit CoreIgnoreListManager(CoreSession *parent);
  ~CoreIgnoreListManager();

  inline virtual const QMetaObject *syncMetaObject() const { return &IgnoreListManager::staticMetaObject; }

//private:
//  void loadDefaults();

};

#endif //COREIGNORELISTMANAGER_H
