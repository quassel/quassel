#pragma once

#include <QDateTime>

#include "common-export.h"
#include "irc/ircmessage.h"

struct COMMON_EXPORT ParsedMessage
{
    QDateTime timestamp;
    QByteArray rawMessage;
    IrcMessage message;
    ParsedMessage(QDateTime timestamp, QByteArray rawMessage, IrcMessage message);

    friend COMMON_EXPORT QDebug operator<<(QDebug dbg, const ParsedMessage& m);
    friend COMMON_EXPORT std::ostream& operator<<(std::ostream& o, const ParsedMessage& m);
};