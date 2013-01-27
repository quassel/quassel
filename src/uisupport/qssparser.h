/***************************************************************************
 *   Copyright (C) 2005-2013 by the Quassel Project                        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef QSSPARSER_H_
#define QSSPARSER_H_

#include "uistyle.h"

class QssParser
{
    Q_DECLARE_TR_FUNCTIONS(QssParser)

public:
    QssParser();

    void processStyleSheet(QString &sheet);

    inline QPalette palette() const { return _palette; }
    inline QVector<QBrush> uiStylePalette() const { return _uiStylePalette; }
    inline const QHash<quint64, QTextCharFormat> &formats() const { return _formats; }
    inline const QHash<quint32, QTextCharFormat> &listItemFormats() const { return _listItemFormats; }

protected:
    typedef QList<qreal> ColorTuple;

    void parseChatLineBlock(const QString &decl, const QString &contents);
    void parsePaletteBlock(const QString &decl, const QString &contents);
    void parseListItemBlock(const QString &decl, const QString &contents);

    quint64 parseFormatType(const QString &decl);
    quint32 parseItemFormatType(const QString &decl);

    QTextCharFormat parseFormat(const QString &qss);

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
    QHash<QString, UiStyle::ColorRole> _uiStyleColorRoles;

private:
    QPalette _palette;
    QVector<QBrush> _uiStylePalette;
    QHash<quint64, QTextCharFormat> _formats;
    QHash<quint32, QTextCharFormat> _listItemFormats;
    int _maxSenderHash;
};


#endif
