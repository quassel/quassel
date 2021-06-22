#pragma once

#include "common-export.h"

#include "irctagkey.h"

struct COMMON_EXPORT IrcMessage {
    QHash<IrcTagKey, QString> tags;
    QString prefix;
    QString cmd;
    QList<QByteArray> params;

    explicit IrcMessage(
        QHash<IrcTagKey, QString> tags,
        QString prefix,
        QString cmd,
        QList<QByteArray> params
    ) :
        tags(std::move(tags)),
        prefix(std::move(prefix)),
        cmd(std::move(cmd)),
        params(std::move(params))
    {}

    friend COMMON_EXPORT bool operator==(const IrcMessage& a, const IrcMessage& b);
    friend COMMON_EXPORT QDebug operator<<(QDebug dbg, const IrcMessage& i);
    friend COMMON_EXPORT std::ostream& operator<<(std::ostream& o, const IrcMessage& i);
};