#pragma once

#include "ircmessage.h"
#include "types.h"

struct Batch
{
    QByteArray key;
    QByteArray type;
    QList<QByteArray> params;
    QHash<IrcTagKey, QString> tags;
    QList<IrcMessage> messages;
    Batch* parent;
    QList<Batch> children;

    explicit Batch(
        QByteArray key,
        QByteArray type,
        QList<QByteArray> params,
        QHash<IrcTagKey, QString> tags
    ) :
        key(std::move(key)),
        type(std::move(type)),
        params(std::move(params)),
        tags(std::move(tags)),
        parent(nullptr)
    {}

    friend void addMessage(Batch& batch, const IrcMessage& message);
    friend void addChild(Batch& batch, Batch& child);

    friend uint qHash(const Batch& batch);
    friend bool operator==(const Batch& a, const Batch& b);
    friend bool operator<(const Batch& a, const Batch& b);
    friend QDebug operator<<(QDebug dbg, const Batch& i);
    friend std::ostream& operator<<(std::ostream& o, const Batch& i);
};