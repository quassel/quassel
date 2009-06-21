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

#ifndef QSSPARSER_H_
#define QSSPARSER_H_

#include "uistyle.h"

class QssParser {
  Q_DECLARE_TR_FUNCTIONS(QssParser)

  public:
    QssParser();

    void loadStyleSheet(const QString &sheet);

    inline QPalette palette() const { return _palette; }
    inline const QHash<quint64, QTextCharFormat>& formats() const { return _formats; }

  protected:
    typedef QList<qreal> ColorTuple;

    void parseChatLineData(const QString &decl, const QString &contents);
    void parsePaletteData(const QString &decl, const QString &contents);

    quint64 parseFormatType(const QString &decl);

    QTextCharFormat parseFormat(const QString &qss);
    bool parsePalette(QPalette &, const QString &qss);

    // Parse color/brush-related properties
    QBrush parseBrush(const QString &str, bool *ok = 0);
    QColor parseColor(const QString &str);
    ColorTuple parseColorTuple(const QString &str);
    QGradientStops parseGradientStops(const QString &str);

    // Parse font-related properties
    void parseFont(const QString &str, QTextCharFormat *format);
    void parseFontStyle(const QString &str, QTextCharFormat *format);
    void parseFontWeight(const QString &str, QTextCharFormat *format);
    void parseFontSize(const QString &str, QTextCharFormat *format);
    void parseFontFamily(const QString &str, QTextCharFormat *format);

    QHash<QString, QPalette::ColorRole> _paletteColorRoles;

  private:
    QPalette _palette;
    QHash<quint64, QTextCharFormat> _formats;
    int _maxSenderHash;
};

#endif
