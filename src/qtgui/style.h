/***************************************************************************
 *   Copyright (C) 2005-07 by The Quassel Team                             *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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

#ifndef _STYLE_H_
#define _STYLE_H_

#include <QtCore>
#include <QtGui>

class Style {

  public:
    static void init();

    struct UrlInfo {
      int start, end;
      QUrl url;
    };

    struct FormattedString {
      QString text;
      QList<QTextLayout::FormatRange> formats;
      QList<UrlInfo> urls;
    };

    static QString mircToInternal(QString);
    //static QString internalToMirc(QString);
    static FormattedString internalToFormatted(QString);
    static int sepTsSender() { return 10; }
    static int sepSenderText() { return 10; }


  private:
    static QHash<QString, QTextCharFormat> formats;
    static QHash<QString, QColor> colors;

};

#endif
