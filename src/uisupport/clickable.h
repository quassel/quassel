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

#ifndef CLICKABLE_H_
#define CLICKABLE_H_

#include <QStackedWidget>

class Clickable {

public:
  // Don't change these enums without also changing dependent methods!
  enum Type {
    Invalid = -1,
    Url = 0,
    Channel = 1,
    Nick = 2
  };

  explicit inline Clickable(Type type = Invalid, quint16 start = 0, quint16 length = 0)
  : _type(type), _start(start), _length(length)
  {}

  inline Type type() const { return _type; }
  inline quint16 start() const { return _start; }
  inline quint16 length() const { return _length; }

  inline bool isValid() const { return _type != Invalid; }

private:
  Type _type;
  quint16 _start;
  quint16 _length;
};

class ClickableList : public QList<Clickable> {

public:
  static ClickableList fromString(const QString &);

};

#endif // CLICKABLE_H_
