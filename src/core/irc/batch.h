#pragma once

#include "ircmessage.h"
#include "types.h"

struct Batch
{
    NetworkId networkId;
    QString key;
    QString type;
    QList<QString> params;
    QList<IrcMessage> messages;
    Batch* parent;
    QList<Batch> children;

    explicit Batch(
        NetworkId networkId,
        QString key,
        QString type,
        QList<QString> params
    ) :
        networkId(networkId),
        key(std::move(key)),
        type(std::move(type)),
        params(std::move(params))
    {}

    friend void addMessage(Batch& batch, const IrcMessage& message);
    friend void addChild(Batch& batch, Batch& child);

    friend uint qHash(const Batch& batch);
    friend bool operator==(const Batch& a, const Batch& b);
    friend bool operator<(const Batch& a, const Batch& b);
    friend QDebug operator<<(QDebug dbg, const Batch& i);
    friend std::ostream& operator<<(std::ostream& o, const Batch& i);
};