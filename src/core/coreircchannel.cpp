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

#include "coreircchannel.h"
#include "corenetwork.h"

INIT_SYNCABLE_OBJECT(CoreIrcChannel)
CoreIrcChannel::CoreIrcChannel(const QString &channelname, Network *network)
  : IrcChannel(channelname, network),
    _receivedWelcomeMsg(false)
{
#ifdef HAVE_QCA2
  _cipher = 0;
#endif
}

CoreIrcChannel::~CoreIrcChannel() {
#ifdef HAVE_QCA2
  delete _cipher;
#endif
}

#ifdef HAVE_QCA2
Cipher *CoreIrcChannel::cipher() const {
  if(!_cipher)
    _cipher = new Cipher();

  return _cipher;
}

void CoreIrcChannel::setEncrypted(bool e) {
  if(!Cipher::neededFeaturesAvailable())
    return;

  if(e) {
    if(topic().isEmpty())
      return;

    QByteArray key = qobject_cast<CoreNetwork *>(network())->cipherKey(name());
    if(key.isEmpty())
      return;

    if(!cipher()->setKey(key))
      return;

    QByteArray decrypted = cipher()->decryptTopic(topic().toAscii());
    setTopic(decodeString(decrypted));
  }
}

#endif
