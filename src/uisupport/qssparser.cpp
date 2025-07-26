/***************************************************************************
 *   Copyright (C) 2005-2022 by the Quassel Project                        *
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

#include "qssparser.h"

#include <tuple>
#include <utility>

#include <QApplication>
#include <QRegularExpressionMatch>

QssParser::QssParser()
{
    _palette = QApplication::palette();

    // Init palette color roles
    _paletteColorRoles["alternate-base"] = QPalette::AlternateBase;
    _paletteColorRoles["background"] = QPalette::Window;
    _paletteColorRoles["base"] = QPalette::Base;
    _paletteColorRoles["bright-text"] = QPalette::BrightText;
    _paletteColorRoles["button"] = QPalette::Button;
    _paletteColorRoles["button-text"] = QPalette::ButtonText;
    _paletteColorRoles["dark"] = QPalette::Dark;
    _paletteColorRoles["foreground"] = QPalette::WindowText;
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

    _uiStylePalette = QVector<QBrush>(static_cast<int>(UiStyle::ColorRole::NumRoles), QBrush());

    _uiStyleColorRoles["marker-line"] = UiStyle::ColorRole::MarkerLine;
    // Sender colors
    _uiStyleColorRoles["sender-color-self"] = UiStyle::ColorRole::SenderColorSelf;
    _uiStyleColorRoles["sender-color-00"] = UiStyle::ColorRole::SenderColor00;
    _uiStyleColorRoles["sender-color-01"] = UiStyle::ColorRole::SenderColor01;
    _uiStyleColorRoles["sender-color-02"] = UiStyle::ColorRole::SenderColor02;
    _uiStyleColorRoles["sender-color-03"] = UiStyle::ColorRole::SenderColor03;
    _uiStyleColorRoles["sender-color-04"] = UiStyle::ColorRole::SenderColor04;
    _uiStyleColorRoles["sender-color-05"] = UiStyle::ColorRole::SenderColor05;
    _uiStyleColorRoles["sender-color-06"] = UiStyle::ColorRole::SenderColor06;
    _uiStyleColorRoles["sender-color-07"] = UiStyle::ColorRole::SenderColor07;
    _uiStyleColorRoles["sender-color-08"] = UiStyle::ColorRole::SenderColor08;
    _uiStyleColorRoles["sender-color-09"] = UiStyle::ColorRole::SenderColor09;
    _uiStyleColorRoles["sender-color-0a"] = UiStyle::ColorRole::SenderColor0a;
    _uiStyleColorRoles["sender-color-0b"] = UiStyle::ColorRole::SenderColor0b;
    _uiStyleColorRoles["sender-color-0c"] = UiStyle::ColorRole::SenderColor0c;
    _uiStyleColorRoles["sender-color-0d"] = UiStyle::ColorRole::SenderColor0d;
    _uiStyleColorRoles["sender-color-0e"] = UiStyle::ColorRole::SenderColor0e;
    _uiStyleColorRoles["sender-color-0f"] = UiStyle::ColorRole::SenderColor0f;
}

void QssParser::processStyleSheet(QString& ss)
{
    if (ss.isEmpty())
        return;

    // Remove C-style comments /* */ or //
    static QRegularExpression commentRx(R"((//.*(\n|$)|/\*[\s\S]*?\*/))");
    commentRx.setPatternOptions(QRegularExpression::MultilineOption);
    ss.remove(commentRx);

    // Palette definitions first, so we can apply roles later on
    static const QRegularExpression paletterx("(Palette[^{]*)\\{([^}]+)\\}");
    int pos = 0;
    QRegularExpressionMatch paletteMatch;
    while ((paletteMatch = paletterx.match(ss, pos)).hasMatch()) {
        parsePaletteBlock(paletteMatch.captured(1).trimmed(), paletteMatch.captured(2).trimmed());
        pos = paletteMatch.capturedStart();
        ss.remove(pos, paletteMatch.capturedLength());
    }

    // Now we can parse the rest of our custom blocks
    static const QRegularExpression blockrx("((?:ChatLine|ChatListItem|NickListItem)[^{]*)\\{([^}]+)\\}");
    pos = 0;
    QRegularExpressionMatch blockMatch;
    while ((blockMatch = blockrx.match(ss, pos)).hasMatch()) {
        QString declaration = blockMatch.captured(1).trimmed();
        QString contents = blockMatch.captured(2).trimmed();

        if (declaration.startsWith("ChatLine"))
            parseChatLineBlock(declaration, contents);
        else if (declaration.startsWith("ChatListItem") || declaration.startsWith("NickListItem"))
            parseListItemBlock(declaration, contents);

        pos = blockMatch.capturedStart();
        ss.remove(pos, blockMatch.capturedLength());
    }

    // Log the final stylesheet for debugging
    if (ss.trimmed().isEmpty()) {
        qWarning() << Q_FUNC_INFO << tr("Processed stylesheet is empty");
    }
    else {
        qDebug() << Q_FUNC_INFO << tr("Processed stylesheet: %1").arg(ss);
    }
}

/******** Parse a whole block: declaration { contents } *******/

void QssParser::parseChatLineBlock(const QString& decl, const QString& contents)
{
    UiStyle::FormatType fmtType;
    UiStyle::MessageLabel label;
    std::tie(fmtType, label) = parseFormatType(decl);
    if (fmtType == UiStyle::FormatType::Invalid)
        return;

    _formats[fmtType | label].merge(parseFormat(contents));
}

void QssParser::parseListItemBlock(const QString& decl, const QString& contents)
{
    UiStyle::ItemFormatType fmtType = parseItemFormatType(decl);
    if (fmtType == UiStyle::ItemFormatType::Invalid)
        return;

    _listItemFormats[fmtType].merge(parseFormat(contents));
}

// Palette { ... } specifies the application palette
// ColorGroups can be specified like pseudo states, chaining is OR (contrary to normal CSS handling):
//   Palette:inactive:disabled { ... } applies to both the Inactive and the Disabled state
void QssParser::parsePaletteBlock(const QString& decl, const QString& contents)
{
    QList<QPalette::ColorGroup> colorGroups;

    // Check if we want to apply this palette definition for particular ColorGroups
    static const QRegularExpression rx("Palette((:(normal|active|inactive|disabled))*)");
    QRegularExpressionMatch match = rx.match(decl);
    if (!match.hasMatch()) {
        qWarning() << Q_FUNC_INFO << tr("Invalid block declaration: %1").arg(decl);
        return;
    }
    if (!match.captured(1).isEmpty()) {
        QStringList groups = match.captured(1).split(':', Qt::SkipEmptyParts);
        for (const QString& g : groups) {
            if ((g == "normal" || g == "active") && !colorGroups.contains(QPalette::Active))
                colorGroups.append(QPalette::Active);
            else if (g == "inactive" && !colorGroups.contains(QPalette::Inactive))
                colorGroups.append(QPalette::Inactive);
            else if (g == "disabled" && !colorGroups.contains(QPalette::Disabled))
                colorGroups.append(QPalette::Disabled);
        }
    }

    // Now let's go through the roles
    for (const QString& line : contents.split(';', Qt::SkipEmptyParts)) {
        QString trimmedLine = line.trimmed();
        // Skip empty lines and comments
        if (trimmedLine.isEmpty() || trimmedLine.startsWith("/*") || trimmedLine.startsWith("//")) {
            continue;
        }
        int idx = trimmedLine.indexOf(':');
        if (idx <= 0) {
            // Only warn about lines that look like they should be role assignments
            if (trimmedLine.contains(':')) {
                qWarning() << Q_FUNC_INFO << tr("Invalid palette role assignment: %1").arg(trimmedLine);
            }
            continue;
        }
        QString rolestr = trimmedLine.left(idx).trimmed();
        QString brushstr = trimmedLine.mid(idx + 1).trimmed();

        if (_paletteColorRoles.contains(rolestr)) {
            QBrush brush = parseBrush(brushstr);
            if (colorGroups.count()) {
                for (QPalette::ColorGroup group : colorGroups)
                    _palette.setBrush(group, _paletteColorRoles.value(rolestr), brush);
            }
            else
                _palette.setBrush(_paletteColorRoles.value(rolestr), brush);
        }
        else if (_uiStyleColorRoles.contains(rolestr)) {
            _uiStylePalette[static_cast<int>(_uiStyleColorRoles.value(rolestr))] = parseBrush(brushstr);
        }
        else {
            // Only warn about role names that don't look like CSS selectors or comments
            if (!rolestr.isEmpty() && !rolestr.contains('{') && !rolestr.contains('}') && !rolestr.startsWith("/*")) {
                qWarning() << Q_FUNC_INFO << tr("Unknown palette role name: %1").arg(rolestr);
            }
        }
    }
}

/******** Determine format types from a block declaration ********/

std::pair<UiStyle::FormatType, UiStyle::MessageLabel> QssParser::parseFormatType(const QString& decl)
{
    using FormatType = UiStyle::FormatType;
    using MessageLabel = UiStyle::MessageLabel;

    const std::pair<UiStyle::FormatType, UiStyle::MessageLabel> invalid{FormatType::Invalid, MessageLabel::None};

    static const QRegularExpression rx(R"(ChatLine(?:::(\w+))?(?:#([\w\-]+))?(?:\[([=,\"\w\s-]+)\])?)");
    if (!rx.isValid()) {
        qWarning() << Q_FUNC_INFO << tr("Invalid QRegularExpression pattern: %1").arg(rx.pattern());
        return invalid;
    }
    QRegularExpressionMatch match = rx.match(decl);
    if (!match.hasMatch()) {
        qWarning() << Q_FUNC_INFO << tr("Invalid block declaration: %1").arg(decl);
        return invalid;
    }
    QString subElement = match.captured(1);
    QString msgType = match.captured(2);
    QString conditions = match.captured(3);

    FormatType fmtType{FormatType::Base};
    MessageLabel label{MessageLabel::None};

    // First determine the subelement
    if (!subElement.isEmpty()) {
        if (subElement == "timestamp")
            fmtType |= FormatType::Timestamp;
        else if (subElement == "sender")
            fmtType |= FormatType::Sender;
        else if (subElement == "nick")
            fmtType |= FormatType::Nick;
        else if (subElement == "contents")
            fmtType |= FormatType::Contents;
        else if (subElement == "hostmask")
            fmtType |= FormatType::Hostmask;
        else if (subElement == "modeflags")
            fmtType |= FormatType::ModeFlags;
        else if (subElement == "url")
            fmtType |= FormatType::Url;
        else {
            qWarning() << Q_FUNC_INFO << tr("Invalid subelement name in %1").arg(decl);
            return invalid;
        }
    }

    // Now, figure out the message type
    if (!msgType.isEmpty()) {
        if (msgType == "plain")
            fmtType |= FormatType::PlainMsg;
        else if (msgType == "notice")
            fmtType |= FormatType::NoticeMsg;
        else if (msgType == "action")
            fmtType |= FormatType::ActionMsg;
        else if (msgType == "nick")
            fmtType |= FormatType::NickMsg;
        else if (msgType == "mode")
            fmtType |= FormatType::ModeMsg;
        else if (msgType == "join")
            fmtType |= FormatType::JoinMsg;
        else if (msgType == "part")
            fmtType |= FormatType::PartMsg;
        else if (msgType == "quit")
            fmtType |= FormatType::QuitMsg;
        else if (msgType == "kick")
            fmtType |= FormatType::KickMsg;
        else if (msgType == "kill")
            fmtType |= FormatType::KillMsg;
        else if (msgType == "server")
            fmtType |= FormatType::ServerMsg;
        else if (msgType == "info")
            fmtType |= FormatType::InfoMsg;
        else if (msgType == "error")
            fmtType |= FormatType::ErrorMsg;
        else if (msgType == "daychange")
            fmtType |= FormatType::DayChangeMsg;
        else if (msgType == "topic")
            fmtType |= FormatType::TopicMsg;
        else if (msgType == "netsplit-join")
            fmtType |= FormatType::NetsplitJoinMsg;
        else if (msgType == "netsplit-quit")
            fmtType |= FormatType::NetsplitQuitMsg;
        else if (msgType == "invite")
            fmtType |= FormatType::InviteMsg;
        else {
            qWarning() << Q_FUNC_INFO << tr("Invalid message type in %1").arg(decl);
        }
    }

    // Next up: conditional (formats, labels, nickhash)
    static const QRegularExpression condRx(R"lit(\s*([\w\-]+)\s*=\s*"(\w+)"\s*)lit");
    QRegularExpressionMatch condMatch;
    if (!conditions.isEmpty()) {
        for (const QString& cond : conditions.split(',', Qt::SkipEmptyParts)) {
            condMatch = condRx.match(cond);
            if (!condMatch.hasMatch()) {
                qWarning() << Q_FUNC_INFO << tr("Invalid condition %1").arg(cond);
                return invalid;
            }
            QString condName = condMatch.captured(1);
            QString condValue = condMatch.captured(2);
            if (condName == "label") {
                if (condValue == "highlight")
                    label |= MessageLabel::Highlight;
                else if (condValue == "selected")
                    label |= MessageLabel::Selected;
                else if (condValue == "hovered")
                    label |= MessageLabel::Hovered;
                else {
                    qWarning() << Q_FUNC_INFO << tr("Invalid message label: %1").arg(condValue);
                    return invalid;
                }
            }
            else if (condName == "sender") {
                if (condValue == "self")
                    label |= MessageLabel::OwnMsg;  // sender="self" is actually treated as a label
                else {
                    bool ok;
                    quint32 val = condValue.toUInt(&ok, 16);
                    if (!ok) {
                        qWarning() << Q_FUNC_INFO << tr("Invalid senderhash specification: %1").arg(condValue);
                        return invalid;
                    }
                    if (val >= 16) {
                        qWarning() << Q_FUNC_INFO << tr("Senderhash can be at most \"0x0f\"!");
                        return invalid;
                    }
                    label |= static_cast<MessageLabel>(++val << 16);
                }
            }
            else if (condName == "format") {
                if (condValue == "bold")
                    fmtType |= FormatType::Bold;
                else if (condValue == "italic")
                    fmtType |= FormatType::Italic;
                else if (condValue == "underline")
                    fmtType |= FormatType::Underline;
                else if (condValue == "strikethrough")
                    fmtType |= FormatType::Strikethrough;
                else {
                    qWarning() << Q_FUNC_INFO << tr("Invalid format name: %1").arg(condValue);
                    return invalid;
                }
            }
            else if (condName == "fg-color" || condName == "bg-color") {
                bool ok;
                quint32 col = condValue.toUInt(&ok, 16);
                if (!ok || col > 0x0f) {
                    qWarning() << Q_FUNC_INFO << tr("Illegal IRC color specification (must be between 00 and 0f): %1").arg(condValue);
                    return invalid;
                }
                if (condName == "fg-color")
                    fmtType |= 0x00400000 | (col << 24);
                else
                    fmtType |= 0x00800000 | (col << 28);
            }
            else {
                qWarning() << Q_FUNC_INFO << tr("Unhandled condition: %1").arg(condName);
                return invalid;
            }
        }
    }

    return std::make_pair(fmtType, label);
}

// FIXME: Code duplication
UiStyle::ItemFormatType QssParser::parseItemFormatType(const QString& decl)
{
    using ItemFormatType = UiStyle::ItemFormatType;

    static const QRegularExpression rx(R"((Chat|Nick)ListItem(?:\[([=,\"\w\s-]+)\])?)");
    if (!rx.isValid()) {
        qWarning() << Q_FUNC_INFO << tr("Invalid QRegularExpression pattern: %1").arg(rx.pattern());
        return ItemFormatType::Invalid;
    }
    QRegularExpressionMatch match = rx.match(decl);
    if (!match.hasMatch()) {
        qWarning() << Q_FUNC_INFO << tr("Invalid block declaration: %1").arg(decl);
        return ItemFormatType::Invalid;
    }
    QString mainItemType = match.captured(1);
    QString properties = match.captured(2);

    ItemFormatType fmtType{ItemFormatType::None};

    // Next up: properties
    QString type, state;
    if (!properties.isEmpty()) {
        QRegularExpressionMatch propMatch;
        QHash<QString, QString> props;
        static const QRegularExpression propRx(R"lit(\s*([\w\-]+)\s*=\s*"([\w\-]+)"\s*)lit");
        for (const QString& prop : properties.split(',', Qt::SkipEmptyParts)) {
            propMatch = propRx.match(prop);
            if (!propMatch.hasMatch()) {
                qWarning() << Q_FUNC_INFO << tr("Invalid proplist %1").arg(prop);
                return ItemFormatType::Invalid;
            }
            props[propMatch.captured(1)] = propMatch.captured(2);
        }
        type = props.value("type");
        state = props.value("state");
    }

    if (mainItemType == "Chat") {
        fmtType |= ItemFormatType::BufferViewItem;
        if (!type.isEmpty()) {
            if (type == "network")
                fmtType |= ItemFormatType::NetworkItem;
            else if (type == "channel")
                fmtType |= ItemFormatType::ChannelBufferItem;
            else if (type == "query")
                fmtType |= ItemFormatType::QueryBufferItem;
            else {
                qWarning() << Q_FUNC_INFO << tr("Invalid chatlist item type %1").arg(type);
                return ItemFormatType::Invalid;
            }
        }
        if (!state.isEmpty()) {
            if (state == "inactive")
                fmtType |= ItemFormatType::InactiveBuffer;
            else if (state == "channel-event")
                fmtType |= ItemFormatType::ActiveBuffer;
            else if (state == "unread-message")
                fmtType |= ItemFormatType::UnreadBuffer;
            else if (state == "highlighted")
                fmtType |= ItemFormatType::HighlightedBuffer;
            else if (state == "away")
                fmtType |= ItemFormatType::UserAway;
            else {
                qWarning() << Q_FUNC_INFO << tr("Invalid chatlist state %1").arg(state);
                return ItemFormatType::Invalid;
            }
        }
    }
    else {  // NickList
        fmtType |= ItemFormatType::NickViewItem;
        if (!type.isEmpty()) {
            if (type == "user") {
                fmtType |= ItemFormatType::IrcUserItem;
                if (state == "away")
                    fmtType |= ItemFormatType::UserAway;
            }
            else if (type == "category")
                fmtType |= ItemFormatType::UserCategoryItem;
        }
    }
    return fmtType;
}

/******** Parse a whole format attribute block ********/

QTextCharFormat QssParser::parseFormat(const QString& qss)
{
    QTextCharFormat format;

    for (const QString& line : qss.split(';', Qt::SkipEmptyParts)) {
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

        // Color code overrides
        else if (property == "allow-foreground-override") {
            bool ok;
            bool v = parseBoolean(value, &ok);
            if (ok)
                format.setProperty(static_cast<int>(UiStyle::FormatProperty::AllowForegroundOverride), v);
        }
        else if (property == "allow-background-override") {
            bool ok;
            bool v = parseBoolean(value, &ok);
            if (ok)
                format.setProperty(static_cast<int>(UiStyle::FormatProperty::AllowBackgroundOverride), v);
        }

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

/******** Boolean value ********/

bool QssParser::parseBoolean(const QString& str, bool* ok) const
{
    if (ok)
        *ok = true;

    if (str == "true")
        return true;
    if (str == "false")
        return false;

    qWarning() << Q_FUNC_INFO << tr("Invalid boolean value: %1").arg(str);
    if (ok)
        *ok = false;
    return false;
}

/******** Brush ********/

QBrush QssParser::parseBrush(const QString& str, bool* ok)
{
    if (ok)
        *ok = false;
    QColor c = parseColor(str);
    if (c.isValid()) {
        if (ok)
            *ok = true;
        return QBrush(c);
    }

    if (str.startsWith("palette")) {  // Palette color role
        static const QRegularExpression rx(R"(palette\s*\(\s*([a-z-0-9]+)\s*\))");
        QRegularExpressionMatch match = rx.match(str);
        if (!match.hasMatch()) {
            qWarning() << Q_FUNC_INFO << tr("Invalid palette color role specification: %1").arg(str);
            return QBrush();
        }
        if (_paletteColorRoles.contains(match.captured(1)))
            return QBrush(_palette.brush(_paletteColorRoles.value(match.captured(1))));
        if (_uiStyleColorRoles.contains(match.captured(1)))
            return QBrush(_uiStylePalette.at(static_cast<int>(_uiStyleColorRoles.value(match.captured(1)))));
        qWarning() << Q_FUNC_INFO << tr("Unknown palette color role: %1").arg(match.captured(1));
        return QBrush();
    }
    else if (str.startsWith("qlineargradient")) {
        static const QString rxFloat(R"(\s*(-?\s*[0-9]*\.?[0-9]+)\s*)");
        static const QRegularExpression rx(QString(R"(qlineargradient\s*\(\s*x1:%1,\s*y1:%1,\s*x2:%1,\s*y2:%1,(.+)\))").arg(rxFloat));
        QRegularExpressionMatch match = rx.match(str);
        if (!match.hasMatch()) {
            qWarning() << Q_FUNC_INFO << tr("Invalid gradient declaration: %1").arg(str);
            return QBrush();
        }
        qreal x1 = match.captured(1).toDouble();
        qreal y1 = match.captured(2).toDouble();
        qreal x2 = match.captured(3).toDouble();
        qreal y2 = match.captured(4).toDouble();
        QGradientStops stops = parseGradientStops(match.captured(5).trimmed());
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
        static const QString rxFloat(R"(\s*(-?\s*[0-9]*\.?[0-9]+)\s*)");
        static const QRegularExpression rx(QString(R"(qconicalgradient\s*\(\s*cx:%1,\s*cy:%1,\s*angle:%1,(.+)\))").arg(rxFloat));
        QRegularExpressionMatch match = rx.match(str);
        if (!match.hasMatch()) {
            qWarning() << Q_FUNC_INFO << tr("Invalid gradient declaration: %1").arg(str);
            return QBrush();
        }
        qreal cx = match.captured(1).toDouble();
        qreal cy = match.captured(2).toDouble();
        qreal angle = match.captured(3).toDouble();
        QGradientStops stops = parseGradientStops(match.captured(4).trimmed());
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
        static const QString rxFloat(R"(\s*(-?\s*[0-9]*\.?[0-9]+)\s*)");
        static const QRegularExpression rx(
            QString(R"(qradialgradient\s*\(\s*cx:%1,\s*cy:%1,\s*radius:%1,\s*fx:%1,\s*fy:%1,(.+)\))").arg(rxFloat));
        QRegularExpressionMatch match = rx.match(str);
        if (!match.hasMatch()) {
            qWarning() << Q_FUNC_INFO << tr("Invalid gradient declaration: %1").arg(str);
            return QBrush();
        }
        qreal cx = match.captured(1).toDouble();
        qreal cy = match.captured(2).toDouble();
        qreal radius = match.captured(3).toDouble();
        qreal fx = match.captured(4).toDouble();
        qreal fy = match.captured(5).toDouble();
        QGradientStops stops = parseGradientStops(match.captured(6).trimmed());
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

QColor QssParser::parseColor(const QString& str)
{
    if (str.toLower() == "transparent") {
        return QColor(Qt::transparent);
    }
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
        static const QRegularExpression rx("#?[0-9A-Fa-f]+");
        QRegularExpressionMatch match = rx.match(str);
        if (match.hasMatch())
            return QColor(str);
    }
    return QColor();
}

// get a list of comma-separated int values or percentages (rel to 0-255)
QssParser::ColorTuple QssParser::parseColorTuple(const QString& str)
{
    ColorTuple result;
    static const QRegularExpression rx(R"(\(((\s*[0-9]{1,3}%?\s*)(,\s*[0-9]{1,3}%?\s*)*)\))");
    QRegularExpressionMatch match = rx.match(str.trimmed());
    if (!match.hasMatch()) {
        return ColorTuple();
    }
    QStringList values = match.captured(1).split(',');
    for (const QString& v : values) {
        qreal val;
        bool perc = false;
        bool ok;
        QString trimmed = v.trimmed();
        if (trimmed.endsWith('%')) {
            perc = true;
            trimmed.chop(1);
        }
        val = (qreal)trimmed.toUInt(&ok);
        if (!ok)
            return ColorTuple();
        if (perc)
            val = 255 * val / 100;
        result.append(val);
    }
    return result;
}

QGradientStops QssParser::parseGradientStops(const QString& str_)
{
    QString str = str_;
    QGradientStops result;
    static const QString rxFloat("(0(?:\\.[0-9]+)?|1(?:\\.0+)?|\\.[0-9]+)");  // values between 0 and 1
    static const QRegularExpression rx(QString(R"(\s*,?\s*stop:\s*(%1)\s+([a-zA-Z#][^,}]*)(\s*,?\s*(?:stop:|$))").arg(rxFloat));
    QRegularExpressionMatch match;
    while ((match = rx.match(str)).hasMatch()) {
        qreal x = match.captured(1).toDouble();
        QString colorStr = match.captured(2).trimmed();
        QColor c = parseColor(colorStr);
        if (!c.isValid()) {
            qWarning() << Q_FUNC_INFO << tr("Invalid color in gradient stop: %1").arg(colorStr);
            return QGradientStops();
        }
        result << QGradientStop(x, c);
        str.remove(0, match.capturedStart() + match.capturedLength());
    }
    return result;
}

/******** Font Properties ********/

void QssParser::parseFont(const QString& value, QTextCharFormat* format)
{
    static const QRegularExpression rx(
        "((?:(?:normal|italic|oblique|underline|strikethrough|bold|100|200|300|400|500|600|700|800|900) ){0,2}) ?(\\d+)(pt|px)? \"(.*)\"");
    QRegularExpressionMatch match = rx.match(value);
    if (!match.hasMatch()) {
        qWarning() << Q_FUNC_INFO << tr("Invalid font specification: %1").arg(value);
        return;
    }
    format->setFontItalic(false);
    format->setFontUnderline(false);
    format->setFontStrikeOut(false);
    format->setFontWeight(QFont::Normal);
    QStringList proplist = match.captured(1).split(' ', Qt::SkipEmptyParts);
    for (const QString& prop : proplist) {
        if (prop == "normal")
            ;  // pass
        else if (prop == "italic")
            format->setFontItalic(true);
        else if (prop == "underline")
            format->setFontUnderline(true);
        else if (prop == "strikethrough")
            format->setFontStrikeOut(true);
        else if (prop == "oblique")
            // Oblique is not a property supported by QTextCharFormat
            format->setFontItalic(true);
        else {  // number
            int w = prop.toInt();
            format->setFontWeight(qMin(w / 8, 99));  // taken from Qt's qss parser
        }
    }

    if (match.captured(3) == "px")
        format->setProperty(QTextFormat::FontPixelSize, match.captured(2).toInt());
    else
        format->setFontPointSize(match.captured(2).toInt());

    format->setFontFamilies(QStringList(match.captured(4)));
}

void QssParser::parseFontStyle(const QString& value, QTextCharFormat* format)
{
    if (value == "normal")
        format->setFontItalic(false);
    else if (value == "italic")
        format->setFontItalic(true);
    else if (value == "underline")
        format->setFontUnderline(true);
    else if (value == "strikethrough")
        format->setFontStrikeOut(true);
    else if (value == "oblique")
        // Oblique is not a property supported by QTextCharFormat
        format->setFontItalic(true);
    else {
        qWarning() << Q_FUNC_INFO << tr("Invalid font style specification: %1").arg(value);
    }
}

void QssParser::parseFontWeight(const QString& value, QTextCharFormat* format)
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
        format->setFontWeight(qMin(w / 8, 99));  // taken from Qt's qss parser
    }
}

void QssParser::parseFontSize(const QString& value, QTextCharFormat* format)
{
    static const QRegularExpression rx("(\\d+)(pt|px)");
    QRegularExpressionMatch match = rx.match(value);
    if (!match.hasMatch()) {
        qWarning() << Q_FUNC_INFO << tr("Invalid font size specification: %1").arg(value);
        return;
    }
    if (match.captured(2) == "px")
        format->setProperty(QTextFormat::FontPixelSize, match.captured(1).toInt());
    else
        format->setFontPointSize(match.captured(1).toInt());
}

void QssParser::parseFontFamily(const QString& value, QTextCharFormat* format)
{
    QString family = value;
    if (family.startsWith('"') && family.endsWith('"')) {
        family = family.mid(1, family.length() - 2);
    }
    format->setFontFamilies(QStringList(family));
}
