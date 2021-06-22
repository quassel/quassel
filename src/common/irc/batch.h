#pragma once

#include "parsedmessage.h"
#include "types.h"

struct Batch
{
    QByteArray key;
    QByteArray type;
    QList<QByteArray> params;
    QHash<IrcTagKey, QString> tags;
    QList<ParsedMessage> messages;
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

    friend COMMON_EXPORT void addMessage(Batch& batch, const ParsedMessage& message);
    friend COMMON_EXPORT void addChild(Batch& batch, Batch& child);

    friend COMMON_EXPORT uint qHash(const Batch& batch);
    friend COMMON_EXPORT bool operator==(const Batch& a, const Batch& b);
    friend COMMON_EXPORT bool operator<(const Batch& a, const Batch& b);
    friend COMMON_EXPORT QDebug operator<<(QDebug dbg, const Batch& i);
    friend COMMON_EXPORT std::ostream& operator<<(std::ostream& o, const Batch& i);
};