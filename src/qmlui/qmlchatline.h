/***************************************************************************
 *   Copyright (C) 2005-2011 by the Quassel Project                        *
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

#ifndef QMLCHATLINE_H_
#define QMLCHATLINE_H_

#include <QDeclarativeItem>

#include "uistyle.h"

class QmlChatLine : public QDeclarativeItem {
  Q_OBJECT

  Q_PROPERTY(QVariant chatLineData READ chatLineData WRITE setChatLineData)

  //Q_PROPERTY(QVariant )

public:
  //! Contains all data needed to render a QmlChatLine
  struct Data {
    struct ChatLineColumnData {
      QString text;
      UiStyle::FormatList formats;
    };

    ChatLineColumnData timestamp;
    ChatLineColumnData sender;
    ChatLineColumnData contents;
  };

  QmlChatLine(QDeclarativeItem *parent = 0);
  virtual ~QmlChatLine();

  inline Data data() const { return _data; }
  inline QVariant chatLineData() const { return QVariant::fromValue<Data>(_data); }
  inline void setChatLineData(const QVariant &data) { _data = data.value<Data>(); }

  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);

  static void registerTypes();

private:
  Data _data;
};

QDataStream &operator<<(QDataStream &out, const QmlChatLine::Data &data);
QDataStream &operator>>(QDataStream &in, QmlChatLine::Data &data);

Q_DECLARE_METATYPE(QmlChatLine::Data)

#endif
