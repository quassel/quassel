#include "batch.h"

uint qHash(const Batch& key)
{
    return qHash(QString("%1 %2")
                 .arg(key.networkId.toInt())
                 .arg(key.key));
}

bool operator==(const Batch& a, const Batch& b)
{
    return a.networkId == b.networkId && a.key == b.key;
}

bool operator<(const Batch& a, const Batch& b)
{
    if (a.networkId == b.networkId) {
        return a.key < b.key;
    } else {
        return a.networkId < b.networkId;
    }
}

QDebug operator<<(QDebug o, const Batch& m) {
    return o << QString("Batch{network=%1, batch=%2, type=%3")
        .arg(m.networkId.toInt())
        .arg(m.key, m.type);
}

std::ostream& operator<<(std::ostream& o, const Batch& m) {
    return o << QString("BatchKey{network=%1, batch=%2, type=%3")
                    .arg(m.networkId.toInt())
                    .arg(m.key, m.type)
                    .toStdString();
}

void addMessage(Batch& batch, const IrcMessage& message) {
    batch.messages << message;
}

void addChild(Batch& batch, Batch& child) {
    batch.children << child;
    child.parent = &batch;
}