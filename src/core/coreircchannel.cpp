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

#include "coreircchannel.h"
#include "corenetwork.h"

CoreIrcChannel::CoreIrcChannel(const QString &channelname, Network *network)
    : IrcChannel(channelname, network),
    _receivedWelcomeMsg(false)
{
#ifdef HAVE_QCA2
    _cipher = nullptr;

    // Get the cipher key from CoreNetwork if present
    CoreNetwork *coreNetwork = qobject_cast<CoreNetwork *>(network);
    if (coreNetwork) {
        QByteArray key = coreNetwork->readChannelCipherKey(channelname);
        if (!key.isEmpty()) {
            setEncrypted(cipher()->setKey(key));
        }
    }
#endif
}


CoreIrcChannel::~CoreIrcChannel()
{
#ifdef HAVE_QCA2
    // Store the cipher key in CoreNetwork, including empty keys if a cipher
    // exists. There is no need to store the empty key if no cipher exists; no
    // key was present when instantiating and no key was set during the
    // channel's lifetime.
    CoreNetwork *coreNetwork = qobject_cast<CoreNetwork *>(network());
    if (coreNetwork && _cipher) {
        coreNetwork->storeChannelCipherKey(name(), _cipher->key());
    }

    delete _cipher;
#endif
}


#ifdef HAVE_QCA2
Cipher *CoreIrcChannel::cipher() const
{
    if (!_cipher)
        _cipher = new Cipher();

    return _cipher;
}


void CoreIrcChannel::setEncrypted(bool e)
{
    IrcChannel::setEncrypted(e);

    if (!Cipher::neededFeaturesAvailable())
        return;

    if (e) {
        if (topic().isEmpty())
            return;

        QByteArray decrypted = cipher()->decryptTopic(topic().toLatin1());
        setTopic(decodeString(decrypted));
    }
}


#endif
