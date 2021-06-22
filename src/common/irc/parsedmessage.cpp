#include "parsedmessage.h"

#include <utility>

ParsedMessage::ParsedMessage(QDateTime timestamp, QByteArray rawMessage, IrcMessage message)
    : timestamp(std::move(timestamp))
    , rawMessage(std::move(rawMessage))
    , message(std::move(message))
{}

QDebug operator<<(QDebug o, const ParsedMessage& m)
{
    return o << "(timestamp=" << m.timestamp
      << ", message=" << m.message
      << ", raw=" << m.rawMessage
      << ")";
}

std::ostream& operator<<(std::ostream& o, const ParsedMessage& m)
{
    return o << "(timestamp=" << m.timestamp.toString().toStdString()
             << ", message=" << m.message
             << ", raw=" << m.rawMessage.toStdString()
             << ")";
}