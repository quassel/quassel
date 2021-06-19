#include "batch.h"

uint qHash(const Batch& key)
{
    return qHash(key.key);
}

bool operator==(const Batch& a, const Batch& b)
{
    return a.key == b.key;
}

bool operator<(const Batch& a, const Batch& b)
{
    return a.key < b.key;
}

QDebug operator<<(QDebug o, const Batch& m) {
    return o.nospace().noquote()
           << "Batch{batch="<< m.key
           << ", type="<< m.type
           << ", tags="<< m.tags
           << "}";
}

std::ostream& operator<<(std::ostream& o, const Batch& m) {
    return o << "Batch{batch="<< m.key.toStdString()
             << ", type="<< m.type.toStdString()
             << "}";
}

void addMessage(Batch& batch, const ParsedMessage& message) {
    batch.messages << message;
}

void addChild(Batch& batch, Batch& child) {
    batch.children << child;
    child.parent = &batch;
}