#pragma once

#include "corenetwork.h"
#include "irctag.h"

struct IrcMessage {
    QDateTime timestamp;
    QByteArray raw;
    QHash<IrcTagKey, QString> tags;
    QString prefix;
    QString cmd;
    QList<QByteArray> params;

    explicit IrcMessage(
        QDateTime timestamp,
        QByteArray raw,
        QHash<IrcTagKey, QString> tags,
        QString prefix,
        QString cmd,
        QList<QByteArray> params
    ) :
        timestamp(std::move(timestamp)),
        raw(std::move(raw)),
        tags(std::move(tags)),
        prefix(std::move(prefix)),
        cmd(std::move(cmd)),
        params(std::move(params))
    {}

    friend QDebug operator<<(QDebug dbg, const IrcMessage& i);
    friend std::ostream& operator<<(std::ostream& o, const IrcMessage& i);
};