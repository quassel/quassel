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

#include "coreircuser.h"
#include "corenetwork.h"

CoreIrcUser::CoreIrcUser(const QString &hostmask, Network *network) : IrcUser(hostmask, network)
{
#ifdef HAVE_QCA2
    _cipher = 0;

    // Get the cipher key from CoreNetwork if present
    CoreNetwork *coreNetwork = qobject_cast<CoreNetwork *>(network);
    if (coreNetwork) {
        QByteArray key = coreNetwork->readChannelCipherKey(nick().toLower());
        if (!key.isEmpty()) {
            if (!_cipher) {
                _cipher = new Cipher();
            }
            setEncrypted(_cipher->setKey(key));
        }
    }
#endif
}


CoreIrcUser::~CoreIrcUser()
{
#ifdef HAVE_QCA2
    // Store the cipher key in CoreNetwork, including empty keys if a cipher
    // exists. There is no need to store the empty key if no cipher exists; no
    // key was present when instantiating and no key was set during the
    // channel's lifetime.
    CoreNetwork *coreNetwork = qobject_cast<CoreNetwork *>(network());
    if (coreNetwork && _cipher) {
        coreNetwork->storeChannelCipherKey(nick().toLower(), _cipher->key());
    }

    delete _cipher;
#endif
}


#ifdef HAVE_QCA2
Cipher *CoreIrcUser::cipher() const
{
    if (!_cipher)
        _cipher = new Cipher();

    return _cipher;
}


#endif
