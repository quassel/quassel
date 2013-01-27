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

#include <QApplication>

#include "qssparser.h"

QssParser::QssParser()
    : _maxSenderHash(0)
{
    _palette = QApplication::palette();

    // Init palette color roles
    _paletteColorRoles["alternate-base"] = QPalette::AlternateBase;
    _paletteColorRoles["background"] = QPalette::Background;
    _paletteColorRoles["base"] = QPalette::Base;
    _paletteColorRoles["bright-text"] = QPalette::BrightText;
    _paletteColorRoles["button"] = QPalette::Button;
    _paletteColorRoles["button-text"] = QPalette::ButtonText;
    _paletteColorRoles["dark"] = QPalette::Dark;
    _paletteColorRoles["foreground"] = QPalette::Foreground;
    _paletteColorRoles["highlight"] = QPalette::Highlight;
    _paletteColorRoles["highlighted-text"] = QPalette::HighlightedText;
    _paletteColorRoles["light"] = QPalette::Light;
    _paletteColorRoles["link"] = QPalette::Link;
    _paletteColorRoles["link-visited"] = QPalette::LinkVisited;
    _paletteColorRoles["mid"] = QPalette::Mid;
    _paletteColorRoles["midlight"] = QPalette::Midlight;
    _paletteColorRoles["shadow"] = QPalette::Shadow;
    _paletteColorRoles["text"] = QPalette::Text;
    _paletteColorRoles["tooltip-base"] = QPalette::ToolTipBase;
    _paletteColorRoles["tooltip-text"] = QPalette::ToolTipText;
    _paletteColorRoles["window"] = QPalette::Window;
    _paletteColorRoles["window-text"] = QPalette::WindowText;

    _uiStylePalette = QVector<QBrush>(UiStyle::NumRoles, QBrush());

    _uiStyleColorRoles["marker-line"] = UiStyle::MarkerLine;
}


void QssParser::processStyleSheet(QString &ss)
{
    if (ss.isEmpty())
        return;

    // Remove C-style comments /* */ or //
    QRegExp commentRx("(//.*(\\n|$)|/\\*.*\\*/)");
    commentRx.setMinimal(true);
    ss.remove(commentRx);

    // Palette definitions first, so we can apply roles later on
    QRegExp paletterx("(Palette[^{]*)\\{([^}]+)\\}");
    int pos = 0;
    while ((pos = paletterx.indexIn(ss, pos)) >= 0) {
        parsePaletteBlock(paletterx.cap(1).trimmed(), paletterx.cap(2).trimmed());
        ss.remove(pos, paletterx.matchedLength());
    }

    // Now we can parse the rest of our custom blocks
    QRegExp blockrx("((?:ChatLine|ChatListItem|NickListItem)[^{]*)\\{([^}]+)\\}");
    pos = 0;
    while ((pos = blockrx.indexIn(ss, pos)) >= 0) {
        //qDebug() << blockrx.cap(1) << blockrx.cap(2);
        QString declaration = blockrx.cap(1).trimmed();
        QString contents = blockrx.cap(2).trimmed();

        if (declaration.startsWith("ChatLine"))
            parseChatLineBlock(declaration, contents);
        else if (declaration.startsWith("ChatListItem") || declaration.startsWith("NickListItem"))
            parseListItemBlock(declaration, contents);
        //else
        // TODO: add moar here

        ss.remove(pos, blockrx.matchedLength());
    }
}


/******** Parse a whole block: declaration { contents } *******/

void QssParser::parseChatLineBlock(const QString &decl, const QString &contents)
{
    quint64 fmtType = parseFormatType(decl);
    if (fmtType == UiStyle::Invalid)
        return;

    _formats[fmtType].merge(parseFormat(contents));
}


void QssParser::parseListItemBlock(const QString &decl, const QString &contents)
{
    quint32 fmtType = parseItemFormatType(decl);
    if (fmtType == UiStyle::Invalid)
        return;

    _listItemFormats[fmtType].merge(parseFormat(contents));
}


// Palette { ... } specifies the application palette
// ColorGroups can be specified like pseudo states, chaining is OR (contrary to normal CSS handling):
//   Palette:inactive:disabled { ... } applies to both the Inactive and the Disabled state
void QssParser::parsePaletteBlock(const QString &decl, const QString &contents)
{
    QList<QPalette::ColorGroup> colorGroups;

    // Check if we want to apply this palette definition for particular ColorGroups
    QRegExp rx("Palette((:(normal|active|inactive|disabled))*)");
    if (!rx.exactMatch(decl)) {
        qWarning() << Q_FUNC_INFO << tr("Invalid block declaration: %1").arg(decl);
        return;
    }
    if (!rx.cap(1).isEmpty()) {
        QStringList groups = rx.cap(1).split(':', QString::SkipEmptyParts);
        foreach(QString g, groups) {
            if ((g == "normal" || g == "active") && !colorGroups.contains(QPalette::Active))
                colorGroups.append(QPalette::Active);
            else if (g == "inactive" && !colorGroups.contains(QPalette::Inactive))
                colorGroups.append(QPalette::Inactive);
            else if (g == "disabled" && !colorGroups.contains(QPalette::Disabled))
                colorGroups.append(QPalette::Disabled);
        }
    }

    // Now let's go through the roles
    foreach(QString line, contents.split(';', QString::SkipEmptyParts)) {
        int idx = line.indexOf(':');
        if (idx <= 0) {
            qWarning() << Q_FUNC_INFO << tr("Invalid palette role assignment: %1").arg(line.trimmed());
            continue;
        }
        QString rolestr = line.left(idx).trimmed();
        QString brushstr = line.mid(idx + 1).trimmed();

        if (_paletteColorRoles.contains(rolestr)) {
            QBrush brush = parseBrush(brushstr);
            if (colorGroups.count()) {
                foreach(QPalette::ColorGroup group, colorGroups)
                _palette.setBrush(group, _paletteColorRoles.value(rolestr), brush);
            }
            else
                _palette.setBrush(_paletteColorRoles.value(rolestr), brush);
        }
        else if (_uiStyleColorRoles.contains(rolestr)) {
            _uiStylePalette[_uiStyleColorRoles.value(rolestr)] = parseBrush(brushstr);
        }
        else
            qWarning() << Q_FUNC_INFO << tr("Unknown palette role name: %1").arg(rolestr);
    }
}


/******** Determine format types from a block declaration ********/

quint64 QssParser::parseFormatType(const QString &decl)
{
    QRegExp rx("ChatLine(?:::(\\w+))?(?:#([\\w\\-]+))?(?:\\[([=-,\\\"\\w\\s]+)\\])?");
    // $1: subelement; $2: msgtype; $3: conditionals
    if (!rx.exactMatch(decl)) {
        qWarning() << Q_FUNC_INFO << tr("Invalid block declaration: %1").arg(decl);
        return UiStyle::Invalid;
    }
    QString subElement = rx.cap(1);
    QString msgType = rx.cap(2);
    QString conditions = rx.cap(3);

    quint64 fmtType = 0;

    // First determine the subelement
    if (!subElement.isEmpty()) {
        if (subElement == "timestamp")
            fmtType |= UiStyle::Timestamp;
        else if (subElement == "sender")
            fmtType |= UiStyle::Sender;
        else if (subElement == "nick")
            fmtType |= UiStyle::Nick;
        else if (subElement == "contents")
            fmtType |= UiStyle::Contents;
        else if (subElement == "hostmask")
            fmtType |= UiStyle::Hostmask;
        else if (subElement == "modeflags")
            fmtType |= UiStyle::ModeFlags;
        else if (subElement == "url")
            fmtType |= UiStyle::Url;
        else {
            qWarning() << Q_FUNC_INFO << tr("Invalid subelement name in %1").arg(decl);
            return UiStyle::Invalid;
        }
    }

    // Now, figure out the message type
    if (!msgType.isEmpty()) {
        if (msgType == "plain")
            fmtType |= UiStyle::PlainMsg;
        else if (msgType == "notice")
            fmtType |= UiStyle::NoticeMsg;
        else if (msgType == "action")
            fmtType |= UiStyle::ActionMsg;
        else if (msgType == "nick")
            fmtType |= UiStyle::NickMsg;
        else if (msgType == "mode")
            fmtType |= UiStyle::ModeMsg;
        else if (msgType == "join")
            fmtType |= UiStyle::JoinMsg;
        else if (msgType == "part")
            fmtType |= UiStyle::PartMsg;
        else if (msgType == "quit")
            fmtType |= UiStyle::QuitMsg;
        else if (msgType == "kick")
            fmtType |= UiStyle::KickMsg;
        else if (msgType == "kill")
            fmtType |= UiStyle::KillMsg;
        else if (msgType == "server")
            fmtType |= UiStyle::ServerMsg;
        else if (msgType == "info")
            fmtType |= UiStyle::InfoMsg;
        else if (msgType == "error")
            fmtType |= UiStyle::ErrorMsg;
        else if (msgType == "daychange")
            fmtType |= UiStyle::DayChangeMsg;
        else if (msgType == "topic")
            fmtType |= UiStyle::TopicMsg;
        else if (msgType == "netsplit-join")
            fmtType |= UiStyle::NetsplitJoinMsg;
        else if (msgType == "netsplit-quit")
            fmtType |= UiStyle::NetsplitQuitMsg;
        else if (msgType == "invite")
            fmtType |= UiStyle::InviteMsg;
        else {
            qWarning() << Q_FUNC_INFO << tr("Invalid message type in %1").arg(decl);
        }
    }

    // Next up: conditional (formats, labels, nickhash)
    QRegExp condRx("\\s*([\\w\\-]+)\\s*=\\s*\"(\\w+)\"\\s*");
    if (!conditions.isEmpty()) {
        foreach(const QString &cond, conditions.split(',', QString::SkipEmptyParts)) {
            if (!condRx.exactMatch(cond)) {
                qWarning() << Q_FUNC_INFO << tr("Invalid condition %1").arg(cond);
                return UiStyle::Invalid;
            }
            QString condName = condRx.cap(1);
            QString condValue = condRx.cap(2);
            if (condName == "label") {
                quint64 labeltype = 0;
                if (condValue == "highlight")
                    labeltype = UiStyle::Highlight;
                else if (condValue == "selected")
                    labeltype = UiStyle::Selected;
                else {
                    qWarning() << Q_FUNC_INFO << tr("Invalid message label: %1").arg(condValue);
                    return UiStyle::Invalid;
                }
                fmtType |= (labeltype << 32);
            }
            else if (condName == "sender") {
                if (condValue == "self")
                    fmtType |= (quint64) UiStyle::OwnMsg << 32;  // sender="self" is actually treated as a label
                else {
                    bool ok = true;
                    quint64 val = condValue.toUInt(&ok, 16);
                    if (!ok) {
                        qWarning() << Q_FUNC_INFO << tr("Invalid senderhash specification: %1").arg(condValue);
                        return UiStyle::Invalid;
                    }
                    if (val >= 16) {
                        qWarning() << Q_FUNC_INFO << tr("Senderhash can be at most \"0x0f\"!");
                        return UiStyle::Invalid;
                    }
                    fmtType |= ++val << 48;
                }
            }
            else if (condName == "format") {
                if (condValue == "bold")
                    fmtType |= UiStyle::Bold;
                else if (condValue == "italic")
                    fmtType |= UiStyle::Italic;
                else if (condValue == "underline")
                    fmtType |= UiStyle::Underline;
                else if (condValue == "reverse")
                    fmtType |= UiStyle::Reverse;
                else {
                    qWarning() << Q_FUNC_INFO << tr("Invalid format name: %1").arg(condValue);
                    return UiStyle::Invalid;
                }
            }
            else if (condName == "fg-color" || condName == "bg-color") {
                bool ok;
                quint8 col = condValue.toUInt(&ok, 16);
                if (!ok || col > 0x0f) {
                    qWarning() << Q_FUNC_INFO << tr("Illegal IRC color specification (must be between 00 and 0f): %1").arg(condValue);
                    return UiStyle::Invalid;
                }
                if (condName == "fg-color")
                    fmtType |= 0x00400000 | (quint32)(col << 24);
                else
                    fmtType |= 0x00800000 | (quint32)(col << 28);
            }
            else {
                qWarning() << Q_FUNC_INFO << tr("Unhandled condition: %1").arg(condName);
                return UiStyle::Invalid;
            }
        }
    }

    return fmtType;
}


// FIXME: Code duplication
quint32 QssParser::parseItemFormatType(const QString &decl)
{
    QRegExp rx("(Chat|Nick)ListItem(?:\\[([=-,\\\"\\w\\s]+)\\])?");
    // $1: item type; $2: properties
    if (!rx.exactMatch(decl)) {
        qWarning() << Q_FUNC_INFO << tr("Invalid block declaration: %1").arg(decl);
        return UiStyle::Invalid;
    }
    QString mainItemType = rx.cap(1);
    QString properties = rx.cap(2);

    quint32 fmtType = 0;

    // Next up: properties
    QString type, state;
    if (!properties.isEmpty()) {
        QHash<QString, QString> props;
        QRegExp propRx("\\s*([\\w\\-]+)\\s*=\\s*\"([\\w\\-]+)\"\\s*");
        foreach(const QString &prop, properties.split(',', QString::SkipEmptyParts)) {
            if (!propRx.exactMatch(prop)) {
                qWarning() << Q_FUNC_INFO << tr("Invalid proplist %1").arg(prop);
                return UiStyle::Invalid;
            }
            props[propRx.cap(1)] = propRx.cap(2);
        }
        type = props.value("type");
        state = props.value("state");
    }

    if (mainItemType == "Chat") {
        fmtType |= UiStyle::BufferViewItem;
        if (!type.isEmpty()) {
            if (type == "network")
                fmtType |= UiStyle::NetworkItem;
            else if (type == "channel")
                fmtType |= UiStyle::ChannelBufferItem;
            else if (type == "query")
                fmtType |= UiStyle::QueryBufferItem;
            else {
                qWarning() << Q_FUNC_INFO << tr("Invalid chatlist item type %1").arg(type);
                return UiStyle::Invalid;
            }
        }
        if (!state.isEmpty()) {
            if (state == "inactive")
                fmtType |= UiStyle::InactiveBuffer;
            else if (state == "channel-event")
                fmtType |= UiStyle::ActiveBuffer;
            else if (state == "unread-message")
                fmtType |= UiStyle::UnreadBuffer;
            else if (state == "highlighted")
                fmtType |= UiStyle::HighlightedBuffer;
            else if (state == "away")
                fmtType |= UiStyle::UserAway;
            else {
                qWarning() << Q_FUNC_INFO << tr("Invalid chatlist state %1").arg(state);
                return UiStyle::Invalid;
            }
        }
    }
    else { // NickList
        fmtType |= UiStyle::NickViewItem;
        if (!type.isEmpty()) {
            if (type == "user") {
                fmtType |= UiStyle::IrcUserItem;
                if (state == "away")
                    fmtType |= UiStyle::UserAway;
            }
            else if (type == "category")
                fmtType |= UiStyle::UserCategoryItem;
        }
    }
    return fmtType;
}


/******** Parse a whole format attribute block ********/

QTextCharFormat QssParser::parseFormat(const QString &qss)
{
    QTextCharFormat format;

    foreach(QString line, qss.split(';', QString::SkipEmptyParts)) {
        int idx = line.indexOf(':');
        if (idx <= 0) {
            qWarning() << Q_FUNC_INFO << tr("Invalid property declaration: %1").arg(line.trimmed());
            continue;
        }
        QString property = line.left(idx).trimmed();
        QString value = line.mid(idx + 1).simplified();

        if (property == "background" || property == "background-color")
            format.setBackground(parseBrush(value));
        else if (property == "foreground" || property == "color")
            format.setForeground(parseBrush(value));

        // font-related properties
        else if (property.startsWith("font")) {
            if (property == "font")
                parseFont(value, &format);
            else if (property == "font-style")
                parseFontStyle(value, &format);
            else if (property == "font-weight")
                parseFontWeight(value, &format);
            else if (property == "font-size")
                parseFontSize(value, &format);
            else if (property == "font-family")
                parseFontFamily(value, &format);
            else {
                qWarning() << Q_FUNC_INFO << tr("Invalid font property: %1").arg(line);
                continue;
            }
        }

        else {
            qWarning() << Q_FUNC_INFO << tr("Unknown ChatLine property: %1").arg(property);
        }
    }

    return format;
}


/******** Brush ********/

QBrush QssParser::parseBrush(const QString &str, bool *ok)
{
    if (ok)
        *ok = false;
    QColor c = parseColor(str);
    if (c.isValid()) {
        if (ok)
            *ok = true;
        return QBrush(c);
    }

    if (str.startsWith("palette")) { // Palette color role
        QRegExp rx("palette\\s*\\(\\s*([a-z-]+)\\s*\\)");
        if (!rx.exactMatch(str)) {
            qWarning() << Q_FUNC_INFO << tr("Invalid palette color role specification: %1").arg(str);
            return QBrush();
        }
        if (_paletteColorRoles.contains(rx.cap(1)))
            return QBrush(_palette.brush(_paletteColorRoles.value(rx.cap(1))));
        if (_uiStyleColorRoles.contains(rx.cap(1)))
            return QBrush(_uiStylePalette.at(_uiStyleColorRoles.value(rx.cap(1))));
        qWarning() << Q_FUNC_INFO << tr("Unknown palette color role: %1").arg(rx.cap(1));
        return QBrush();
    }
    else if (str.startsWith("qlineargradient")) {
        static QString rxFloat("\\s*(-?\\s*[0-9]*\\.?[0-9]+)\\s*");
        QRegExp rx(QString("qlineargradient\\s*\\(\\s*x1:%1,\\s*y1:%1,\\s*x2:%1,\\s*y2:%1,(.+)\\)").arg(rxFloat));
        if (!rx.exactMatch(str)) {
            qWarning() << Q_FUNC_INFO << tr("Invalid gradient declaration: %1").arg(str);
            return QBrush();
        }
        qreal x1 = rx.cap(1).toDouble();
        qreal y1 = rx.cap(2).toDouble();
        qreal x2 = rx.cap(3).toDouble();
        qreal y2 = rx.cap(4).toDouble();
        QGradientStops stops = parseGradientStops(rx.cap(5).trimmed());
        if (!stops.count()) {
            qWarning() << Q_FUNC_INFO << tr("Invalid gradient stops list: %1").arg(str);
            return QBrush();
        }
        QLinearGradient gradient(x1, y1, x2, y2);
        gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
        gradient.setStops(stops);
        if (ok)
            *ok = true;
        return QBrush(gradient);
    }
    else if (str.startsWith("qconicalgradient")) {
        static QString rxFloat("\\s*(-?\\s*[0-9]*\\.?[0-9]+)\\s*");
        QRegExp rx(QString("qconicalgradient\\s*\\(\\s*cx:%1,\\s*cy:%1,\\s*angle:%1,(.+)\\)").arg(rxFloat));
        if (!rx.exactMatch(str)) {
            qWarning() << Q_FUNC_INFO << tr("Invalid gradient declaration: %1").arg(str);
            return QBrush();
        }
        qreal cx = rx.cap(1).toDouble();
        qreal cy = rx.cap(2).toDouble();
        qreal angle = rx.cap(3).toDouble();
        QGradientStops stops = parseGradientStops(rx.cap(4).trimmed());
        if (!stops.count()) {
            qWarning() << Q_FUNC_INFO << tr("Invalid gradient stops list: %1").arg(str);
            return QBrush();
        }
        QConicalGradient gradient(cx, cy, angle);
        gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
        gradient.setStops(stops);
        if (ok)
            *ok = true;
        return QBrush(gradient);
    }
    else if (str.startsWith("qradialgradient")) {
        static QString rxFloat("\\s*(-?\\s*[0-9]*\\.?[0-9]+)\\s*");
        QRegExp rx(QString("qradialgradient\\s*\\(\\s*cx:%1,\\s*cy:%1,\\s*radius:%1,\\s*fx:%1,\\s*fy:%1,(.+)\\)").arg(rxFloat));
        if (!rx.exactMatch(str)) {
            qWarning() << Q_FUNC_INFO << tr("Invalid gradient declaration: %1").arg(str);
            return QBrush();
        }
        qreal cx = rx.cap(1).toDouble();
        qreal cy = rx.cap(2).toDouble();
        qreal radius = rx.cap(3).toDouble();
        qreal fx = rx.cap(4).toDouble();
        qreal fy = rx.cap(5).toDouble();
        QGradientStops stops = parseGradientStops(rx.cap(6).trimmed());
        if (!stops.count()) {
            qWarning() << Q_FUNC_INFO << tr("Invalid gradient stops list: %1").arg(str);
            return QBrush();
        }
        QRadialGradient gradient(cx, cy, radius, fx, fy);
        gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
        gradient.setStops(stops);
        if (ok)
            *ok = true;
        return QBrush(gradient);
    }

    return QBrush();
}


QColor QssParser::parseColor(const QString &str)
{
    if (str.startsWith("rgba")) {
        ColorTuple tuple = parseColorTuple(str.mid(4));
        if (tuple.count() == 4)
            return QColor(tuple.at(0), tuple.at(1), tuple.at(2), tuple.at(3));
    }
    else if (str.startsWith("rgb")) {
        ColorTuple tuple = parseColorTuple(str.mid(3));
        if (tuple.count() == 3)
            return QColor(tuple.at(0), tuple.at(1), tuple.at(2));
    }
    else if (str.startsWith("hsva")) {
        ColorTuple tuple = parseColorTuple(str.mid(4));
        if (tuple.count() == 4) {
            QColor c;
            c.setHsvF(tuple.at(0), tuple.at(1), tuple.at(2), tuple.at(3));
            return c;
        }
    }
    else if (str.startsWith("hsv")) {
        ColorTuple tuple = parseColorTuple(str.mid(3));
        if (tuple.count() == 3) {
            QColor c;
            c.setHsvF(tuple.at(0), tuple.at(1), tuple.at(2));
            return c;
        }
    }
    else {
        QRegExp rx("#?[0-9A-Fa-z]+");
        if (rx.exactMatch(str))
            return QColor(str);
    }
    return QColor();
}


// get a list of comma-separated int values or percentages (rel to 0-255)
QssParser::ColorTuple QssParser::parseColorTuple(const QString &str)
{
    ColorTuple result;
    QRegExp rx("\\(((\\s*[0-9]{1,3}%?\\s*)(,\\s*[0-9]{1,3}%?\\s*)*)\\)");
    if (!rx.exactMatch(str.trimmed())) {
        return ColorTuple();
    }
    QStringList values = rx.cap(1).split(',');
    foreach(QString v, values) {
        qreal val;
        bool perc = false;
        bool ok;
        v = v.trimmed();
        if (v.endsWith('%')) {
            perc = true;
            v.chop(1);
        }
        val = (qreal)v.toUInt(&ok);
        if (!ok)
            return ColorTuple();
        if (perc)
            val = 255 * val/100;
        result.append(val);
    }
    return result;
}


QGradientStops QssParser::parseGradientStops(const QString &str_)
{
    QString str = str_;
    QGradientStops result;
    static QString rxFloat("(0?\\.[0-9]+|[01])"); // values between 0 and 1
    QRegExp rx(QString("\\s*,?\\s*stop:\\s*(%1)\\s+([^:]+)(,\\s*stop:|$)").arg(rxFloat));
    int idx;
    while ((idx = rx.indexIn(str)) == 0) {
        qreal x = rx.cap(1).toDouble();
        QColor c = parseColor(rx.cap(3));
        if (!c.isValid())
            return QGradientStops();
        result << QGradientStop(x, c);
        str.remove(0, rx.matchedLength() - rx.cap(4).length());
    }
    if (!str.trimmed().isEmpty())
        return QGradientStops();

    return result;
}


/******** Font Properties ********/

void QssParser::parseFont(const QString &value, QTextCharFormat *format)
{
    QRegExp rx("((?:(?:normal|italic|oblique|underline|bold|100|200|300|400|500|600|700|800|900) ){0,2}) ?(\\d+)(pt|px)? \"(.*)\"");
    if (!rx.exactMatch(value)) {
        qWarning() << Q_FUNC_INFO << tr("Invalid font specification: %1").arg(value);
        return;
    }
    format->setFontItalic(false);
    format->setFontWeight(QFont::Normal);
    QStringList proplist = rx.cap(1).split(' ', QString::SkipEmptyParts);
    foreach(QString prop, proplist) {
        if (prop == "italic")
            format->setFontItalic(true);
        else if (prop == "underline")
            format->setFontUnderline(true);
        //else if(prop == "oblique")
        //  format->setStyle(QFont::StyleOblique);
        else if (prop == "bold")
            format->setFontWeight(QFont::Bold);
        else { // number
            int w = prop.toInt();
            format->setFontWeight(qMin(w / 8, 99)); // taken from Qt's qss parser
        }
    }

    if (rx.cap(3) == "px")
        format->setProperty(QTextFormat::FontPixelSize, rx.cap(2).toInt());
    else
        format->setFontPointSize(rx.cap(2).toInt());

    format->setFontFamily(rx.cap(4));
}


void QssParser::parseFontStyle(const QString &value, QTextCharFormat *format)
{
    if (value == "normal")
        format->setFontItalic(false);
    else if (value == "italic")
        format->setFontItalic(true);
    else if (value == "underline")
        format->setFontUnderline(true);
    //else if(value == "oblique")
    //  format->setStyle(QFont::StyleOblique);
    else {
        qWarning() << Q_FUNC_INFO << tr("Invalid font style specification: %1").arg(value);
    }
}


void QssParser::parseFontWeight(const QString &value, QTextCharFormat *format)
{
    if (value == "normal")
        format->setFontWeight(QFont::Normal);
    else if (value == "bold")
        format->setFontWeight(QFont::Bold);
    else {
        bool ok;
        int w = value.toInt(&ok);
        if (!ok) {
            qWarning() << Q_FUNC_INFO << tr("Invalid font weight specification: %1").arg(value);
            return;
        }
        format->setFontWeight(qMin(w / 8, 99)); // taken from Qt's qss parser
    }
}


void QssParser::parseFontSize(const QString &value, QTextCharFormat *format)
{
    QRegExp rx("(\\d+)(pt|px)");
    if (!rx.exactMatch(value)) {
        qWarning() << Q_FUNC_INFO << tr("Invalid font size specification: %1").arg(value);
        return;
    }
    if (rx.cap(2) == "px")
        format->setProperty(QTextFormat::FontPixelSize, rx.cap(1).toInt());
    else
        format->setFontPointSize(rx.cap(1).toInt());
}


void QssParser::parseFontFamily(const QString &value, QTextCharFormat *format)
{
    QString family = value;
    if (family.startsWith('"') && family.endsWith('"')) {
        family = family.mid(1, family.length() - 2);
    }
    format->setFontFamily(family);
}
