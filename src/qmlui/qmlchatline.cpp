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
#include <QPainter>

#include "qmlchatline.h"

void QmlChatLine::registerTypes() {
  qRegisterMetaType<Data>("QmlChatLine::Data");
  qRegisterMetaTypeStreamOperators<Data>("QmlChatLine::Data");
  qmlRegisterType<QmlChatLine>("eu.quassel.qmlui", 1, 0, "ChatLine");
}

QDataStream &operator<<(QDataStream &out, const QmlChatLine::Data &data) {
  out << data.timestamp.text << data.timestamp.formats
      << data.sender.text << data.sender.formats
      << data.contents.text << data.contents.formats;
  return out;
}

QDataStream &operator>>(QDataStream &in, QmlChatLine::Data &data) {
  in >> data.timestamp.text >> data.timestamp.formats
     >> data.sender.text >> data.sender.formats
     >> data.contents.text >> data.contents.formats;
  return in;
}

QmlChatLine::QmlChatLine(QDeclarativeItem *parent) : QDeclarativeItem(parent) {
  setFlag(ItemHasNoContents, false);
  setImplicitHeight(20);
  setImplicitWidth(100);
}

QmlChatLine::~QmlChatLine() {

}

void QmlChatLine::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
  painter->drawText(0, 0, data().contents.text);

}


