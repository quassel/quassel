#include "ircmessage.h"

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