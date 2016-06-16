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

#ifndef IRCCAP_H
#define IRCCAP_H

#include <QString>
#include <QStringList>

// Why a namespace instead of a class?  Seems to be a better fit for C++ than a 'static' class, as
// compared to C# or Java.  However, feel free to change if needed.
// See https://stackoverflow.com/questions/482745/namespaces-for-enum-types-best-practices
/**
 * IRCv3 capability names and values
 */
namespace IrcCap {

    // NOTE: If you add or modify the constants below, update the knownCaps list.

    /**
     * Account change notification.
     *
     * http://ircv3.net/specs/extensions/account-notify-3.1.html
     */
    const QString ACCOUNT_NOTIFY = "account-notify";

    /**
     * Magic number for WHOX, used to ignore user-requested WHOX replies from servers
     *
     * If a user initiates a WHOX, there's no easy way to tell what fields were requested.  It's
     * simpler to not attempt to parse data from user-requested WHOX replies.
     */
    const uint ACCOUNT_NOTIFY_WHOX_NUM = 369;

    /**
     * Away change notification.
     *
     * http://ircv3.net/specs/extensions/away-notify-3.1.html
     */
    const QString AWAY_NOTIFY = "away-notify";

    /**
     * Capability added/removed notification.
     *
     * This is implicitly enabled via CAP LS 302, and is here for servers that only partially
     * support IRCv3.2.
     *
     * http://ircv3.net/specs/extensions/cap-notify-3.2.html
     */
    const QString CAP_NOTIFY = "cap-notify";

    /**
     * Hostname/user changed notification.
     *
     * http://ircv3.net/specs/extensions/chghost-3.2.html
     */
    const QString CHGHOST = "chghost";

    /**
     * Extended join information.
     *
     * http://ircv3.net/specs/extensions/extended-join-3.1.html
     */
    const QString EXTENDED_JOIN = "extended-join";

    /**
     * Multiple mode prefixes in MODE and WHO replies.
     *
     * http://ircv3.net/specs/extensions/multi-prefix-3.1.html
     */
    const QString MULTI_PREFIX = "multi-prefix";

    /**
     * SASL authentication.
     *
     * http://ircv3.net/specs/extensions/sasl-3.2.html
     */
    const QString SASL = "sasl";

    /**
     * Userhost in names replies.
     *
     * http://ircv3.net/specs/extensions/userhost-in-names-3.2.html
     */
    const QString USERHOST_IN_NAMES = "userhost-in-names";

    /**
     * Vendor-specific capabilities
     */
    namespace Vendor {

        /**
         * Self message support, as recognized by ZNC.
         *
         * Some servers (e.g. Bitlbee) assume self-message support; ZNC requires a capability
         * instead.  As self-message is already implemented, there's little reason to not do this.
         *
         * More information in the IRCv3 commit that removed the 'self-message' capability.
         *
         * https://github.com/ircv3/ircv3-specifications/commit/1bfba47843c2526707c902034b3395af934713c8
         */
        const QString ZNC_SELF_MESSAGE = "znc.in/self-message";
    }

    /**
     * List of capabilities currently implemented and requested during capability negotiation.
     */
    const QStringList knownCaps = QStringList {
            ACCOUNT_NOTIFY,
            AWAY_NOTIFY,
            CAP_NOTIFY,
            CHGHOST,
            EXTENDED_JOIN,
            MULTI_PREFIX,
            SASL,
            USERHOST_IN_NAMES,
            Vendor::ZNC_SELF_MESSAGE
    };
    // NOTE: If you modify the knownCaps list, update the constants above as needed.

    /**
     * SASL authentication mechanisms
     *
     * http://ircv3.net/specs/extensions/sasl-3.1.html
     */
    namespace SaslMech {

        /**
         * Check if the given authentication mechanism is likely to be supported.
         *
         * @param[in] saslCapValue   QString of SASL capability value, e.g. capValue(IrcCap::SASL)
         * @param[in] saslMechanism  Desired SASL mechanism
         * @return True if mechanism supported or unknown, otherwise false
         */
        inline bool maybeSupported(const QString &saslCapValue, const QString &saslMechanism) { return
                    ((saslCapValue.length() == 0) || (saslCapValue.contains(saslMechanism, Qt::CaseInsensitive))); }
        // SASL mechanisms are only specified in capability values as part of SASL 3.2.  In
        // SASL 3.1, it's handled differently.  If we don't know via capability value, assume it's
        // supported to reduce the risk of breaking existing setups.
        // See: http://ircv3.net/specs/extensions/sasl-3.1.html
        // And: http://ircv3.net/specs/extensions/sasl-3.2.html

        /**
         * PLAIN authentication, e.g. hashed password
         */
        const QString PLAIN = "PLAIN";

        /**
         * EXTERNAL authentication, e.g. SSL certificate and keys
         */
        const QString EXTERNAL = "EXTERNAL";
    }
}

#endif // IRCCAP_H
