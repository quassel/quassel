/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) version 3.                                           *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef TYPES_H_
#define TYPES_H_

#include <QDebug>
#include <QString>
#include <QVariant>
#include <QTextStream>

class SignedId {
  protected:
    qint32 id;

  public:
    inline SignedId(int _id = 0) { id = _id; }
    inline qint32 toInt() const { return id; }
    inline bool isValid() const { return id > 0; }

    inline bool operator==(const SignedId &other) const { return id == other.id; }
    inline bool operator!=(const SignedId &other) const { return id != other.id; }
    inline bool operator<(const SignedId &other) const { return id < other.id; }
    inline bool operator<=(const SignedId &other) const { return id <= other.id; }
    inline bool operator>(const SignedId &other) const { return id > other.id; }
    inline bool operator>=(const SignedId &other) const { return id >= other.id; }
    inline bool operator==(int i) const { return id == i; }
    inline bool operator!=(int i) const { return id != i; }
    inline bool operator<(int i) const { return id < i; }
    inline bool operator>(int i) const { return id > i; }
    inline bool operator<=(int i) const { return id <= i; }

    inline SignedId operator++(int) { id++; return *this; }
    //inline operator int() const { return toInt(); } // no automatic conversion!

    friend QDataStream &operator>>(QDataStream &in, SignedId &signedId);
};

inline QDataStream &operator<<(QDataStream &out, const SignedId &signedId) { out << signedId.toInt(); return out; }
inline QDataStream &operator>>(QDataStream &in, SignedId &signedId) { in >> signedId.id; return in; }
inline QTextStream &operator<<(QTextStream &out, const SignedId &signedId) { out << QString::number(signedId.toInt()); return out; }
inline QDebug operator<<(QDebug dbg, const SignedId &signedId) { dbg.space() << signedId.toInt(); return dbg; }
inline uint qHash(const SignedId &id) { return qHash(id.toInt()); }

struct UserId : public SignedId {
  inline UserId(int _id = 0) : SignedId(_id) {};
  //inline operator QVariant() const { return QVariant::fromValue<UserId>(*this); }  // no automatic conversion!
};

struct MsgId : public SignedId {
  inline MsgId(int _id = 0) : SignedId(_id) {};
  //inline operator QVariant() const { return QVariant::fromValue<MsgId>(*this); }
};

struct BufferId : public SignedId {
  inline BufferId(int _id = 0) : SignedId(_id) {};
  //inline operator QVariant() const { return QVariant::fromValue<BufferId>(*this); }
};

struct NetworkId : public SignedId {
  inline NetworkId(int _id = 0) : SignedId(_id) {};
  //inline operator QVariant() const { return QVariant::fromValue<NetworkId>(*this); }
};

struct IdentityId : public SignedId {
  inline IdentityId(int _id = 0) : SignedId(_id) {};
  //inline operator QVariant() const { return QVariant::fromValue<IdentityId>(*this); }
};

struct AccountId : public SignedId {
  inline AccountId(int _id = 0) : SignedId(_id) {};
};

Q_DECLARE_METATYPE(UserId)
Q_DECLARE_METATYPE(MsgId)
Q_DECLARE_METATYPE(BufferId)
Q_DECLARE_METATYPE(NetworkId)
Q_DECLARE_METATYPE(IdentityId)
Q_DECLARE_METATYPE(AccountId)

// a few typedefs
typedef QList<MsgId> MsgIdList;
typedef QList<BufferId> BufferIdList;

//! Base class for exceptions.
struct Exception {
  Exception(QString msg = "Unknown Exception") : _msg(msg) {};
  virtual ~Exception() {}; // make gcc happy
  virtual inline QString msg() { return _msg; }

  protected:
    QString _msg;
};

#endif
