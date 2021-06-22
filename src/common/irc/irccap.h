/***************************************************************************
 *   Copyright (C) 2005-2020 by the Quassel Project                        *
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
     * Send account information as a tag with all commands sent by a user.
     *
     * http://ircv3.net/specs/extensions/account-notify-3.1.html
     */
    const QString ACCOUNT_TAG = "account-tag";

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
     * Server sending own messages back.
     *
     * https://ircv3.net/specs/extensions/echo-message-3.2.html
     */
    const QString ECHO_MESSAGE = "echo-message";

    /**
     * Extended join information.
     *
     * http://ircv3.net/specs/extensions/extended-join-3.1.html
     */
    const QString EXTENDED_JOIN = "extended-join";

    /**
     * Standardized invite notifications.
     *
     * https://ircv3.net/specs/extensions/invite-notify-3.2
     */
    const QString INVITE_NOTIFY = "invite-notify";

    /**
     * Additional metadata on a per-message basis
     *
     * https://ircv3.net/specs/extensions/message-tags
     */
    const QString MESSAGE_TAGS = "message-tags";

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
     * Allows updating realname without reconnecting
     *
     * https://ircv3.net/specs/extensions/setname
     */
    const QString SETNAME = "setname";

    /**
     * Userhost in names replies.
     *
     * http://ircv3.net/specs/extensions/userhost-in-names-3.2.html
     */
    const QString USERHOST_IN_NAMES = "userhost-in-names";

    /**
     * Server time for messages.
     *
     * https://ircv3.net/specs/extensions/server-time-3.2.html
     */
    const QString SERVER_TIME = "server-time";

    /**
     * Message batches.
     *
     * https://ircv3.net/specs/extensions/batch
     */
    const QString BATCH = "batch";

    /**
     * Vendor-specific capabilities
     */
    namespace Vendor {

        /**
         * Twitch.tv membership message support
         *
         * User list in a channel can be quite large and often non required for bot users and is then optional.
         *
         * From Twitch.tv documentation:
         * "Adds membership state event data. By default, we do not send this data to clients without this capability."
         *
         * https://dev.twitch.tv/docs/v5/guides/irc/#twitch-irc-capability-membership
         */
        const QString TWITCH_MEMBERSHIP = "twitch.tv/membership";

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
    const QStringList knownCaps = QStringList{ACCOUNT_NOTIFY,
                                              ACCOUNT_TAG,
                                              AWAY_NOTIFY,
                                              CAP_NOTIFY,
                                              CHGHOST,
                                              //ECHO_MESSAGE, // Postponed for message pending UI with batch + labeled-response
                                              EXTENDED_JOIN,
                                              INVITE_NOTIFY,
                                              MESSAGE_TAGS,
                                              MULTI_PREFIX,
                                              SASL,
                                              SETNAME,
                                              USERHOST_IN_NAMES,
                                              SERVER_TIME,
                                              BATCH,
                                              Vendor::TWITCH_MEMBERSHIP,
                                              Vendor::ZNC_SELF_MESSAGE};
    // NOTE: If you modify the knownCaps list, update the constants above as needed.

    /**
     * SASL authentication mechanisms
     *
     * http://ircv3.net/specs/extensions/sasl-3.1.html
     */
    namespace SaslMech {
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
