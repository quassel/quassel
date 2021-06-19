#include "ircmessage.h"

bool operator==(const IrcMessage& a, const IrcMessage& b)
{
    return a.tags == b.tags
               && a.prefix == b.prefix
               && a.cmd == b.cmd
               && a.params == b.params;
}

QDebug operator<<(QDebug o, const IrcMessage& m)
{
    bool autoInsert = o.autoInsertSpaces();
    o.setAutoInsertSpaces(false);
    o << "(tags={";
    QHashIterator<IrcTagKey, QString> tagIterator(m.tags);
    while (tagIterator.hasNext()) {
        tagIterator.next();
        o << tagIterator.key()
          << "='"
          << tagIterator.value()
          << "', ";
    }
    o << "}, prefix="
      << m.prefix
      << ", cmd="
      << m.cmd
      << ", params=[";
    for (const QByteArray& param : m.params) {
        o << "'"
          << param
          << "', ";
    }
    o << "])";
    o.setAutoInsertSpaces(autoInsert);
    return o;
}

std::ostream& operator<<(std::ostream& o, const IrcMessage& m)
{
    o << "(tags={";
    QHashIterator<IrcTagKey, QString> tagIterator(m.tags);
    while (tagIterator.hasNext()) {
        tagIterator.next();
        o << tagIterator.key() << "='" << tagIterator.value().toStdString() << "', ";
    }
    o << "}, prefix="
      << m.prefix.toStdString()
      << ", cmd=" << m.cmd.toStdString()
      << ", params=[";
    for (const QByteArray& param : m.params) {
        o << "'"
          << param.toStdString()
          << "', ";
    }
    o << "])";
    return o;
}