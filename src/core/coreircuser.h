/***************************************************************************
 *   Copyright (C) 2005-2010 by the Quassel Project                        *
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

#ifndef COREIRCUSER_H_
#define COREIRCUSER_H_

#include "ircuser.h"

#ifdef HAVE_QCA2
#  include "cipher.h"
#endif

class CoreIrcUser : public IrcUser {
  SYNCABLE_OBJECT
  Q_OBJECT

public:
  CoreIrcUser(const QString &hostmask, Network *network);
  virtual ~CoreIrcUser();

  inline virtual const QMetaObject *syncMetaObject() const { return &IrcUser::staticMetaObject; }

#ifdef HAVE_QCA2
  Cipher *cipher() const;
  void setEncrypted(bool);
#endif

#ifdef HAVE_QCA2
private:
  mutable Cipher *_cipher;
#endif
};

#endif
