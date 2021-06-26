/***************************************************************************
 *   Copyright (C) 2005-2020 by the Quassel Project                        *
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

#include "uistyle.h"

#include <utility>
#include <vector>

#include <QApplication>
#include <QColor>

#include "buffersettings.h"
#include "icon.h"
#include "qssparser.h"
#include "quassel.h"
#include "uisettings.h"
#include "util.h"

QHash<QString, UiStyle::FormatType> UiStyle::_formatCodes;
bool UiStyle::_useCustomTimestampFormat;                  /// If true, use the custom timestamp format
QString UiStyle::_timestampFormatString;                  /// Timestamp format
QString UiStyle::_systemTimestampFormatString;            /// Cached copy of system locale timestamp format
UiStyle::SenderPrefixMode UiStyle::_senderPrefixDisplay;  /// Display of prefix modes before sender
bool UiStyle::_showSenderBrackets;                        /// If true, show brackets around sender names

namespace {

// Extended mIRC colors as defined in https://modern.ircdocs.horse/formatting.html#colors-16-98
QColor extendedMircColor(int number)
{
    static const std::vector<QColor> colorMap = {"#470000", "#472100", "#474700", "#324700", "#004700", "#00472c", "#004747", "#002747",
                                                 "#000047", "#2e0047", "#470047", "#47002a", "#740000", "#743a00", "#747400", "#517400",
                                                 "#007400", "#007449", "#007474", "#004074", "#000074", "#4b0074", "#740074", "#740045",
                                                 "#b50000", "#b56300", "#b5b500", "#7db500", "#00b500", "#00b571", "#00b5b5", "#0063b5",
                                                 "#0000b5", "#7500b5", "#b500b5", "#b5006b", "#ff0000", "#ff8c00", "#ffff00", "#b2ff00",
                                                 "#00ff00", "#00ffa0", "#00ffff", "#008cff", "#0000ff", "#a500ff", "#ff00ff", "#ff0098",
                                                 "#ff5959", "#ffb459", "#ffff71", "#cfff60", "#6fff6f", "#65ffc9", "#6dffff", "#59b4ff",
                                                 "#5959ff", "#c459ff", "#ff66ff", "#ff59bc", "#ff9c9c", "#ffd39c", "#ffff9c", "#e2ff9c",
                                                 "#9cff9c", "#9cffdb", "#9cffff", "#9cd3ff", "#9c9cff", "#dc9cff", "#ff9cff", "#ff94d3",
                                                 "#000000", "#131313", "#282828", "#363636", "#4d4d4d", "#656565", "#818181", "#9f9f9f",
                                                 "#bcbcbc", "#e2e2e2", "#ffffff"};
    if (number < 16)
        return {};
    size_t index = number - 16;
    return (index < colorMap.size() ? colorMap[index] : QColor{});
}

}  // namespace

UiStyle::UiStyle(QObject* parent)
    : QObject(parent)
    , _channelJoinedIcon{icon::get("irc-channel-active")}
    , _channelPartedIcon{icon::get("irc-channel-inactive")}
    , _userOfflineIcon{icon::get({"im-user-offline", "user-offline"})}
    , _userOnlineIcon{icon::get({"im-user-online", "im-user", "user-available"})}
    , _userAwayIcon{icon::get({"im-user-away", "user-away"})}
    , _categoryOpIcon{icon::get("irc-operator")}
    , _categoryVoiceIcon{icon::get("irc-voice")}
    , _opIconLimit{UserCategoryItem::categoryFromModes("o")}
    , _voiceIconLimit{UserCategoryItem::categoryFromModes("v")}
{
    static bool registered = []() {
        qRegisterMetaType<FormatList>();
        qRegisterMetaTypeStreamOperators<FormatList>();
        return true;
    }();
    Q_UNUSED(registered)

    _uiStylePalette = QVector<QBrush>(static_cast<int>(ColorRole::NumRoles), QBrush());

    // Now initialize the mapping between FormatCodes and FormatTypes...
    _formatCodes["%O"] = FormatType::Base;
    _formatCodes["%B"] = FormatType::Bold;
    _formatCodes["%I"] = FormatType::Italic;
    _formatCodes["%U"] = FormatType::Underline;
    _formatCodes["%S"] = FormatType::Strikethrough;

    _formatCodes["%DN"] = FormatType::Nick;
    _formatCodes["%DH"] = FormatType::Hostmask;
    _formatCodes["%DC"] = FormatType::ChannelName;
    _formatCodes["%DM"] = FormatType::ModeFlags;
    _formatCodes["%DU"] = FormatType::Url;

    // Initialize fallback defaults
    // NOTE: If you change this, update qtui/chatviewsettings.h, too.  More explanations available
    // in there.
    setUseCustomTimestampFormat(false);
    setTimestampFormatString(" hh:mm:ss");
    setSenderPrefixDisplay(UiStyle::SenderPrefixMode::HighestMode);
    enableSenderBrackets(false);

    // BufferView / NickView settings
    UiStyleSettings s;
    s.initAndNotify("ShowItemViewIcons", this, &UiStyle::showItemViewIconsChanged, true);
    s.initAndNotify("AllowMircColors", this, &UiStyle::allowMircColorsChanged, true);

    loadStyleSheet();
}

UiStyle::~UiStyle()
{
    qDeleteAll(_metricsCache);
}

void UiStyle::reload()
{
    loadStyleSheet();
}

void UiStyle::loadStyleSheet()
{
    qDeleteAll(_metricsCache);
    _metricsCache.clear();
    _formatCache.clear();
    _formats.clear();

    UiStyleSettings s;

    QString styleSheet;
    styleSheet += loadStyleSheet("file:///" + Quassel::findDataFilePath("stylesheets/default.qss"));
    styleSheet += loadStyleSheet("file:///" + Quassel::configDirPath() + "settings.qss");
    if (s.value("UseCustomStyleSheet", false).toBool()) {
        QString customSheetPath(s.value("CustomStyleSheetPath").toString());
        QString customSheet = loadStyleSheet("file:///" + customSheetPath, true);
        if (customSheet.isEmpty()) {
            // MIGRATION: changed default install path for data from /usr/share/apps to /usr/share
            if (customSheetPath.startsWith("/usr/share/apps/quassel")) {
                customSheetPath.replace(QRegExp("^/usr/share/apps"), "/usr/share");
                customSheet = loadStyleSheet("file:///" + customSheetPath, true);
                if (!customSheet.isEmpty()) {
                    s.setValue("CustomStyleSheetPath", customSheetPath);
                    qDebug() << "Custom stylesheet path migrated to" << customSheetPath;
                }
            }
        }
        styleSheet += customSheet;
    }
    styleSheet += loadStyleSheet("file:///" + Quassel::optionValue("qss"), true);

    if (!styleSheet.isEmpty()) {
        QssParser parser;
        parser.processStyleSheet(styleSheet);
        QApplication::setPalette(parser.palette());

        _uiStylePalette = parser.uiStylePalette();
        _formats = parser.formats();
        _listItemFormats = parser.listItemFormats();

        styleSheet = styleSheet.trimmed();
        if (!styleSheet.isEmpty())
            qApp->setStyleSheet(styleSheet);  // pass the remaining sections to the application
    }

    emit changed();
}

QString UiStyle::loadStyleSheet(const QString& styleSheet, bool shouldExist)
{
    QString ss = styleSheet;
    if (ss.startsWith("file:///")) {
        ss.remove(0, 8);
        if (ss.isEmpty())
            return QString();

        QFile file(ss);
        if (file.open(QFile::ReadOnly)) {
            QTextStream stream(&file);
            ss = stream.readAll();
            file.close();
        }
        else {
            if (shouldExist)
                qWarning() << "Could not open stylesheet file:" << file.fileName();
            return QString();
        }
    }
    return ss;
}

void UiStyle::updateSystemTimestampFormat()
{
    // Does the system locale use AM/PM designators?  For example:
    // AM/PM:    h:mm AP
    // AM/PM:    hh:mm a
    // 24-hour:  h:mm
    // 24-hour:  hh:mm ADD things
    // For timestamp format, see https://doc.qt.io/qt-5/qdatetime.html#toString
    // This won't update if the system locale is changed while Quassel is running.  If need be,
    // Quassel could hook into notifications of changing system locale to update this.
    //
    // Match any AP or A designation if on a word boundary, including underscores.
    //   .*(\b|_)(A|AP)(\b|_).*
    //   .*         Match any number of characters
    //   \b         Match a word boundary, i.e. "AAA.BBB", "." is matched
    //   _          Match the literal character '_' (not considered a word boundary)
    //   (X|Y)  Match either X or Y, exactly
    //
    // Note that '\' must be escaped as '\\'
    // QRegExp does not support (?> ...), so it's replaced with standard matching, (...)
    // Helpful interactive website for debugging and explaining:  https://regex101.com/
    const QRegExp regExpMatchAMPM(".*(\\b|_)(A|AP)(\\b|_).*", Qt::CaseInsensitive);

    if (regExpMatchAMPM.exactMatch(QLocale().timeFormat(QLocale::ShortFormat))) {
        // AM/PM style used
        _systemTimestampFormatString = " h:mm:ss ap";
    }
    else {
        // 24-hour style used
        _systemTimestampFormatString = " hh:mm:ss";
    }
    // Include a space to give the timestamp a small bit of padding between the border of the chat
    // buffer window and the numbers.  Helps with readability.
    // If you change this to include brackets, e.g. "[hh:mm:ss]", also update
    // ChatScene::updateTimestampHasBrackets() to true or false as needed!
}

// FIXME The following should trigger a reload/refresh of the chat view.
void UiStyle::setUseCustomTimestampFormat(bool enabled)
{
    if (_useCustomTimestampFormat != enabled) {
        _useCustomTimestampFormat = enabled;
    }
}

void UiStyle::setTimestampFormatString(const QString& format)
{
    if (_timestampFormatString != format) {
        _timestampFormatString = format;
    }
}

void UiStyle::setSenderPrefixDisplay(UiStyle::SenderPrefixMode mode)
{
    if (_senderPrefixDisplay != mode) {
        _senderPrefixDisplay = mode;
    }
}

void UiStyle::enableSenderBrackets(bool enabled)
{
    if (_showSenderBrackets != enabled) {
        _showSenderBrackets = enabled;
    }
}

void UiStyle::allowMircColorsChanged(const QVariant& v)
{
    _allowMircColors = v.toBool();
    emit changed();
}

/******** ItemView Styling *******/

void UiStyle::showItemViewIconsChanged(const QVariant& v)
{
    _showBufferViewIcons = _showNickViewIcons = v.toBool();
}

QVariant UiStyle::bufferViewItemData(const QModelIndex& index, int role) const
{
    BufferInfo::Type type = (BufferInfo::Type)index.data(NetworkModel::BufferTypeRole).toInt();
    bool isActive = index.data(NetworkModel::ItemActiveRole).toBool();

    if (role == Qt::DecorationRole) {
        if (!_showBufferViewIcons)
            return QVariant();

        switch (type) {
        case BufferInfo::ChannelBuffer:
            if (isActive)
                return _channelJoinedIcon;
            else
                return _channelPartedIcon;
        case BufferInfo::QueryBuffer:
            if (!isActive)
                return _userOfflineIcon;
            if (index.data(NetworkModel::UserAwayRole).toBool())
                return _userAwayIcon;
            else
                return _userOnlineIcon;
        default:
            return QVariant();
        }
    }

    ItemFormatType fmtType = ItemFormatType::BufferViewItem;
    switch (type) {
    case BufferInfo::StatusBuffer:
        fmtType |= ItemFormatType::NetworkItem;
        break;
    case BufferInfo::ChannelBuffer:
        fmtType |= ItemFormatType::ChannelBufferItem;
        break;
    case BufferInfo::QueryBuffer:
        fmtType |= ItemFormatType::QueryBufferItem;
        break;
    default:
        return QVariant();
    }

    QTextCharFormat fmt = _listItemFormats.value(ItemFormatType::BufferViewItem);
    fmt.merge(_listItemFormats.value(fmtType));

    BufferInfo::ActivityLevel activity = (BufferInfo::ActivityLevel)index.data(NetworkModel::BufferActivityRole).toInt();
    if (activity & BufferInfo::Highlight) {
        fmt.merge(_listItemFormats.value(ItemFormatType::BufferViewItem | ItemFormatType::HighlightedBuffer));
        fmt.merge(_listItemFormats.value(fmtType | ItemFormatType::HighlightedBuffer));
    }
    else if (activity & BufferInfo::NewMessage) {
        fmt.merge(_listItemFormats.value(ItemFormatType::BufferViewItem | ItemFormatType::UnreadBuffer));
        fmt.merge(_listItemFormats.value(fmtType | ItemFormatType::UnreadBuffer));
    }
    else if (activity & BufferInfo::OtherActivity) {
        fmt.merge(_listItemFormats.value(ItemFormatType::BufferViewItem | ItemFormatType::ActiveBuffer));
        fmt.merge(_listItemFormats.value(fmtType | ItemFormatType::ActiveBuffer));
    }
    else if (!isActive) {
        fmt.merge(_listItemFormats.value(ItemFormatType::BufferViewItem | ItemFormatType::InactiveBuffer));
        fmt.merge(_listItemFormats.value(fmtType | ItemFormatType::InactiveBuffer));
    }
    else if (index.data(NetworkModel::UserAwayRole).toBool()) {
        fmt.merge(_listItemFormats.value(ItemFormatType::BufferViewItem | ItemFormatType::UserAway));
        fmt.merge(_listItemFormats.value(fmtType | ItemFormatType::UserAway));
    }

    return itemData(role, fmt);
}

QVariant UiStyle::nickViewItemData(const QModelIndex& index, int role) const
{
    NetworkModel::ItemType type = (NetworkModel::ItemType)index.data(NetworkModel::ItemTypeRole).toInt();

    if (role == Qt::DecorationRole) {
        if (!_showNickViewIcons)
            return QVariant();

        switch (type) {
        case NetworkModel::UserCategoryItemType: {
            int categoryId = index.data(TreeModel::SortRole).toInt();
            if (categoryId <= _opIconLimit)
                return _categoryOpIcon;
            if (categoryId <= _voiceIconLimit)
                return _categoryVoiceIcon;
            return _userOnlineIcon;
        }
        case NetworkModel::IrcUserItemType:
            if (index.data(NetworkModel::ItemActiveRole).toBool())
                return _userOnlineIcon;
            else
                return _userAwayIcon;
        default:
            return QVariant();
        }
    }

    QTextCharFormat fmt = _listItemFormats.value(ItemFormatType::NickViewItem);

    switch (type) {
    case NetworkModel::IrcUserItemType:
        fmt.merge(_listItemFormats.value(ItemFormatType::NickViewItem | ItemFormatType::IrcUserItem));
        if (!index.data(NetworkModel::ItemActiveRole).toBool()) {
            fmt.merge(_listItemFormats.value(ItemFormatType::NickViewItem | ItemFormatType::UserAway));
            fmt.merge(_listItemFormats.value(ItemFormatType::NickViewItem | ItemFormatType::IrcUserItem | ItemFormatType::UserAway));
        }
        break;
    case NetworkModel::UserCategoryItemType:
        fmt.merge(_listItemFormats.value(ItemFormatType::NickViewItem | ItemFormatType::UserCategoryItem));
        break;
    default:
        return QVariant();
    }

    return itemData(role, fmt);
}

QVariant UiStyle::itemData(int role, const QTextCharFormat& format) const
{
    switch (role) {
    case Qt::FontRole:
        return format.font();
    case Qt::ForegroundRole:
        return format.property(QTextFormat::ForegroundBrush);
    case Qt::BackgroundRole:
        return format.property(QTextFormat::BackgroundBrush);
    default:
        return QVariant();
    }
}

/******** Caching *******/

QTextCharFormat UiStyle::parsedFormat(quint64 key) const
{
    return _formats.value(key, QTextCharFormat());
}

namespace {

// Create unique key for given Format object and message label
QString formatKey(const UiStyle::Format& format, UiStyle::MessageLabel label)
{
    return QString::number(format.type | label, 16) + (format.foreground.isValid() ? format.foreground.name() : "#------")
           + (format.background.isValid() ? format.background.name() : "#------");
}

}  // namespace

QTextCharFormat UiStyle::cachedFormat(const Format& format, MessageLabel messageLabel) const
{
    return _formatCache.value(formatKey(format, messageLabel), QTextCharFormat());
}

void UiStyle::setCachedFormat(const QTextCharFormat& charFormat, const Format& format, MessageLabel messageLabel) const
{
    _formatCache[formatKey(format, messageLabel)] = charFormat;
}

QFontMetricsF* UiStyle::fontMetrics(FormatType ftype, MessageLabel label) const
{
    // QFontMetricsF is not assignable, so we need to store pointers :/
    quint64 key = ftype | label;

    if (_metricsCache.contains(key))
        return _metricsCache.value(key);

    return (_metricsCache[key] = new QFontMetricsF(format({ftype, {}, {}}, label).font()));
}

/******** Generate formats ********/

// NOTE: This and the following functions are intimately tied to the values in FormatType. Don't change this
//       until you _really_ know what you do!
QTextCharFormat UiStyle::format(const Format& format, MessageLabel label) const
{
    if (format.type == FormatType::Invalid)
        return {};

    // Check if we have exactly this format readily cached already
    QTextCharFormat charFormat = cachedFormat(format, label);
    if (charFormat.properties().count())
        return charFormat;

    // Merge all formats except mIRC and extended colors
    mergeFormat(charFormat, format, label & 0xffff0000);  // keep nickhash in label
    for (quint32 mask = 0x00000001; mask <= static_cast<quint32>(MessageLabel::Last); mask <<= 1) {
        if (static_cast<quint32>(label) & mask) {
            mergeFormat(charFormat, format, label & (mask | 0xffff0000));
        }
    }

    // Merge mIRC and extended colors, if appropriate. These override any color set previously in the format,
    // unless the AllowForegroundOverride or AllowBackgroundOverride properties are set (via stylesheet).
    if (_allowMircColors) {
        mergeColors(charFormat, format, MessageLabel::None);
        for (quint32 mask = 0x00000001; mask <= static_cast<quint32>(MessageLabel::Last); mask <<= 1) {
            if (static_cast<quint32>(label) & mask) {
                mergeColors(charFormat, format, label & mask);
            }
        }
    }

    setCachedFormat(charFormat, format, label);
    return charFormat;
}

void UiStyle::mergeFormat(QTextCharFormat& charFormat, const Format& format, MessageLabel label) const
{
    mergeSubElementFormat(charFormat, format.type & 0x00ff, label);

    // TODO: allow combinations for mirc formats and colors (each), e.g. setting a special format for "bold and italic"
    //       or "foreground 01 and background 03"
    if ((format.type & 0xfff00) != FormatType::Base) {  // element format
        for (quint32 mask = 0x00100; mask <= 0x80000; mask <<= 1) {
            if ((format.type & mask) != FormatType::Base) {
                mergeSubElementFormat(charFormat,
                    format.type & (mask | 0xff), label);
            }
        }
    }
}

// Merge a subelement format into an existing message format
void UiStyle::mergeSubElementFormat(QTextCharFormat& fmt, FormatType ftype, MessageLabel label) const
{
    quint64 key = ftype | label;
    fmt.merge(parsedFormat(key & 0x0000ffffffffff00ull));  // label + subelement
    fmt.merge(parsedFormat(key & 0x0000ffffffffffffull));  // label + subelement + msgtype
    fmt.merge(parsedFormat(key & 0xffffffffffffff00ull));  // label + subelement + nickhash
    fmt.merge(parsedFormat(key & 0xffffffffffffffffull));  // label + subelement + nickhash + msgtype
}

void UiStyle::mergeColors(QTextCharFormat& charFormat, const Format& format, MessageLabel label) const
{
    bool allowFg = charFormat.property(static_cast<int>(FormatProperty::AllowForegroundOverride)).toBool();
    bool allowBg = charFormat.property(static_cast<int>(FormatProperty::AllowBackgroundOverride)).toBool();

    // Classic mIRC colors (styleable)
    // We assume that those can't be combined with subelement and message types.
    if (allowFg && (format.type & 0x00400000) != FormatType::Base)
        charFormat.merge(parsedFormat((format.type & 0x0f400000) | label));  // foreground
    if (allowBg && (format.type & 0x00800000) != FormatType::Base)
        charFormat.merge(parsedFormat((format.type & 0xf0800000) | label));  // background
    if (allowFg && allowBg && (format.type & 0x00c00000) == static_cast<FormatType>(0x00c00000))
        charFormat.merge(parsedFormat((format.type & 0xffc00000) | label));  // combination

    // Extended mIRC colors (hardcoded)
    if (allowFg && format.foreground.isValid())
        charFormat.setForeground(format.foreground);
    if (allowBg && format.background.isValid())
        charFormat.setBackground(format.background);
}

UiStyle::FormatType UiStyle::formatType(Message::Type msgType)
{
    switch (msgType) {
    case Message::Plain:
        return FormatType::PlainMsg;
    case Message::Notice:
        return FormatType::NoticeMsg;
    case Message::Action:
        return FormatType::ActionMsg;
    case Message::Nick:
        return FormatType::NickMsg;
    case Message::Mode:
        return FormatType::ModeMsg;
    case Message::Join:
        return FormatType::JoinMsg;
    case Message::Part:
        return FormatType::PartMsg;
    case Message::Quit:
        return FormatType::QuitMsg;
    case Message::Kick:
        return FormatType::KickMsg;
    case Message::Kill:
        return FormatType::KillMsg;
    case Message::Server:
        return FormatType::ServerMsg;
    case Message::Info:
        return FormatType::InfoMsg;
    case Message::Error:
        return FormatType::ErrorMsg;
    case Message::DayChange:
        return FormatType::DayChangeMsg;
    case Message::Topic:
        return FormatType::TopicMsg;
    case Message::NetsplitJoin:
        return FormatType::NetsplitJoinMsg;
    case Message::NetsplitQuit:
        return FormatType::NetsplitQuitMsg;
    case Message::Invite:
        return FormatType::InviteMsg;
    }
    // Q_ASSERT(false); // we need to handle all message types
    qWarning() << Q_FUNC_INFO << "Unknown message type:" << msgType;
    return FormatType::ErrorMsg;
}

UiStyle::FormatType UiStyle::formatType(const QString& code)
{
    if (_formatCodes.contains(code))
        return _formatCodes.value(code);
    return FormatType::Invalid;
}

QString UiStyle::formatCode(FormatType ftype)
{
    return _formatCodes.key(ftype);
}

UiStyle::FormatContainer UiStyle::toTextLayoutList(const FormatList& formatList, int textLength, MessageLabel messageLabel) const
{
    UiStyle::FormatContainer formatRanges;
    QTextLayout::FormatRange range;
    size_t i = 0;
    for (i = 0; i < formatList.size(); i++) {
        range.format = format(formatList.at(i).second, messageLabel);
        range.start = formatList.at(i).first;
        if (i > 0)
            formatRanges.last().length = range.start - formatRanges.last().start;
        formatRanges.append(range);
    }
    if (i > 0)
        formatRanges.last().length = textLength - formatRanges.last().start;
    return formatRanges;
}

// This method expects a well-formatted string, there is no error checking!
// Since we create those ourselves, we should be pretty safe that nobody does something crappy here.
UiStyle::StyledString UiStyle::styleString(const QString& s_, FormatType baseFormat)
{
    QString s = s_;
    StyledString result;
    result.formatList.emplace_back(std::make_pair(quint16{0}, Format{baseFormat, {}, {}}));

    if (s.length() > 65535) {
        // We use quint16 for indexes
        qWarning() << QString("String too long to be styled: %1").arg(s);
        result.plainText = s;
        return result;
    }

    Format curfmt{baseFormat, {}, {}};
    QChar fgChar{'f'};  // character to indicate foreground color, changed when reversing

    int pos = 0;
    quint16 length = 0;
    for (;;) {
        pos = s.indexOf('%', pos);
        if (pos < 0)
            break;
        if (s[pos + 1] == '%') {  // escaped %, we just remove one and continue
            s.remove(pos, 1);
            pos++;
            continue;
        }
        if (s[pos + 1] == 'D' && s[pos + 2] == 'c') {  // mIRC color code
            if (s[pos + 3] == '-') {                   // color off
                curfmt.type &= 0x003fffff;
                curfmt.foreground = QColor{};
                curfmt.background = QColor{};
                length = 4;
            }
            else {
                quint32 color = 10 * s[pos + 4].digitValue() + s[pos + 5].digitValue();
                // Color values 0-15 are traditional mIRC colors, defined in the stylesheet and thus going through the format engine
                // Larger color values are hardcoded and applied separately (cf. https://modern.ircdocs.horse/formatting.html#colors-16-98)
                if (s[pos + 3] == fgChar) {
                    if (color < 16) {
                        // Traditional mIRC color, defined in the stylesheet
                        curfmt.type &= 0xf0ffffff;
                        curfmt.type |= color << 24 | 0x00400000;
                        curfmt.foreground = QColor{};
                    }
                    else {
                        curfmt.type &= 0xf0bfffff;  // mask out traditional foreground color
                        curfmt.foreground = extendedMircColor(color);
                    }
                }
                else {
                    if (color < 16) {
                        curfmt.type &= 0x0fffffff;
                        curfmt.type |= color << 28 | 0x00800000;
                        curfmt.background = QColor{};
                    }
                    else {
                        curfmt.type &= 0x0f7fffff;  // mask out traditional background color
                        curfmt.background = extendedMircColor(color);
                    }
                }
                length = 6;
            }
        }
        else if (s[pos + 1] == 'D' && s[pos + 2] == 'h') {  // Hex color
            QColor color{s.mid(pos + 4, 7)};
            if (s[pos + 3] == fgChar) {
                curfmt.type &= 0xf0bfffff;  // mask out mIRC foreground color
                curfmt.foreground = std::move(color);
            }
            else {
                curfmt.type &= 0x0f7fffff;  // mask out mIRC background color
                curfmt.background = std::move(color);
            }
            length = 11;
        }
        else if (s[pos + 1] == 'O') {   // reset formatting
            curfmt.type &= 0x000000ff;  // we keep message type-specific formatting
            curfmt.foreground = QColor{};
            curfmt.background = QColor{};
            fgChar = 'f';
            length = 2;
        }
        else if (s[pos + 1] == 'R') {  // Reverse colors
            fgChar = (fgChar == 'f' ? 'b' : 'f');
            auto orig = static_cast<quint32>(curfmt.type & 0xffc00000);
            curfmt.type &= 0x003fffff;
            curfmt.type |= (orig & 0x00400000) << 1;
            curfmt.type |= (orig & 0x0f000000) << 4;
            curfmt.type |= (orig & 0x00800000) >> 1;
            curfmt.type |= (orig & 0xf0000000) >> 4;
            std::swap(curfmt.foreground, curfmt.background);
            length = 2;
        }
        else {  // all others are toggles
            QString code = QString("%") + s[pos + 1];
            if (s[pos + 1] == 'D')
                code += s[pos + 2];
            FormatType ftype = formatType(code);
            if (ftype == FormatType::Invalid) {
                pos++;
                qWarning() << (QString("Invalid format code in string: %1").arg(s));
                continue;
            }
            curfmt.type ^= ftype;
            length = code.length();
        }
        s.remove(pos, length);
        if (pos == result.formatList.back().first)
            result.formatList.back().second = curfmt;
        else
            result.formatList.emplace_back(std::make_pair(pos, curfmt));
    }
    result.plainText = s;
    return result;
}

QString UiStyle::mircToInternal(const QString& mirc_)
{
    QString mirc;
    mirc.reserve(mirc_.size());
    foreach (const QChar& c, mirc_) {
        if ((c < '\x20' || c == '\x7f') && c != '\x03' && c != '\x04') {
            switch (c.unicode()) {
            case '\x02':
                mirc += "%B";
                break;
            case '\x0f':
                mirc += "%O";
                break;
            case '\x09':
                mirc += "        ";
                break;
            case '\x11':
                // Monospace not supported yet
                break;
            case '\x12':
            case '\x16':
                mirc += "%R";
                break;
            case '\x1d':
                mirc += "%I";
                break;
            case '\x1e':
                mirc += "%S";
                break;
            case '\x1f':
                mirc += "%U";
                break;
            case '\x7f':
                mirc += QChar(0x2421);
                break;
            default:
                mirc += QChar(0x2400 + c.unicode());
            }
        }
        else {
            if (c == '%')
                mirc += c;
            mirc += c;
        }
    }

    // Now we bring the color codes (\x03) in a sane format that can be parsed more easily later.
    // %Dcfxx is foreground, %Dcbxx is background color, where xx is a 2 digit dec number denoting the color code.
    // %Dc- turns color off.
    // Note: We use the "mirc standard" as described in <http://www.mirc.co.uk/help/color.txt>.
    //       This means that we don't accept something like \x03,5 (even though others, like WeeChat, do).
    {
        int pos = 0;
        while (true) {
            pos = mirc.indexOf('\x03', pos);
            if (pos < 0)
                break;  // no more mirc color codes
            QString ins, num;
            int l = mirc.length();
            int i = pos + 1;
            // check for fg color
            if (i < l && mirc[i].isDigit()) {
                num = mirc[i++];
                if (i < l && mirc[i].isDigit())
                    num.append(mirc[i++]);
                else
                    num.prepend('0');
                ins = QString("%Dcf%1").arg(num);

                if (i + 1 < l && mirc[i] == ',' && mirc[i + 1].isDigit()) {
                    i++;
                    num = mirc[i++];
                    if (i < l && mirc[i].isDigit())
                        num.append(mirc[i++]);
                    else
                        num.prepend('0');
                    ins += QString("%Dcb%1").arg(num);
                }
            }
            else {
                ins = "%Dc-";
            }
            mirc.replace(pos, i - pos, ins);
        }
    }

    // Hex colors, as specified in https://modern.ircdocs.horse/formatting.html#hex-color
    // %Dhf#rrggbb is foreground, %Dhb#rrggbb is background
    {
        static const QRegExp rx{"[\\da-fA-F]{6}"};
        int pos = 0;
        while (true) {
            if (pos >= mirc.length())
                break;
            pos = mirc.indexOf('\x04', pos);
            if (pos < 0)
                break;
            int i = pos + 1;
            QString ins;
            auto num = mirc.mid(i, 6);
            if (!num.isEmpty() && rx.exactMatch(num)) {
                ins = "%Dhf#" + num.toLower();
                i += 6;
                if (i < mirc.length() && mirc[i] == ',' && !(num = mirc.mid(i + 1, 6)).isEmpty() && rx.exactMatch(num)) {
                    ins += "%Dhb#" + num.toLower();
                    i += 7;
                }
            }
            else {
                ins = "%Dc-";
            }
            mirc.replace(pos, i - pos, ins);
            pos += ins.length();
        }
    }

    return mirc;
}

QString UiStyle::systemTimestampFormatString()
{
    if (_systemTimestampFormatString.isEmpty()) {
        // Calculate and cache the system timestamp format string
        updateSystemTimestampFormat();
    }
    return _systemTimestampFormatString;
}

QString UiStyle::timestampFormatString()
{
    if (useCustomTimestampFormat()) {
        return _timestampFormatString;
    }
    else {
        return systemTimestampFormatString();
    }
}

/***********************************************************************************/
UiStyle::StyledMessage::StyledMessage(const Message& msg)
    : Message(msg)
{
    switch (type()) {
    // Don't compute the sender hash for message types without a nickname embedded
    case Message::Server:
    case Message::Info:
    case Message::Error:
    case Message::DayChange:
    case Message::Topic:
    case Message::Invite:
    // Don't compute the sender hash for messages with multiple nicks
    // Fixing this without breaking themes would be.. complex.
    case Message::NetsplitJoin:
    case Message::NetsplitQuit:
    case Message::Kick:
    // Don't compute the sender hash for message types that are not yet completed elsewhere
    case Message::Kill:
        _senderHash = 0x00;
        break;
    default:
        // Compute the sender hash for all other message types
        _senderHash = 0xff;
        break;
    }
}

void UiStyle::StyledMessage::style() const
{
    QString user = userFromMask(sender());
    QString host = hostFromMask(sender());
    QString nick = nickFromMask(sender());
    QString txt = UiStyle::mircToInternal(contents());
    QString bufferName = bufferInfo().bufferName();
    bufferName.replace('%', "%%");  // well, you _can_ have a % in a buffername apparently... -_-
    host.replace('%', "%%");        // hostnames too...
    user.replace('%', "%%");        // and the username...
    nick.replace('%', "%%");        // ... and then there's totally RFC-violating servers like justin.tv m(
    const int maxNetsplitNicks = 15;

    QString t;
    switch (type()) {
    case Message::Plain:
        t = QString("%1").arg(txt);
        break;
    case Message::Notice:
        t = QString("%1").arg(txt);
        break;
    case Message::Action:
        t = QString("%DN%1%DN %2").arg(nick).arg(txt);
        break;
    case Message::Nick:
        //: Nick Message
        if (nick == contents())
            t = tr("You are now known as %DN%1%DN").arg(txt);
        else
            t = tr("%DN%1%DN is now known as %DN%2%DN").arg(nick, txt);
        break;
    case Message::Mode:
        //: Mode Message
        if (nick.isEmpty())
            t = tr("User mode: %DM%1%DM").arg(txt);
        else
            t = tr("Mode %DM%1%DM by %DN%2%DN").arg(txt, nick);
        break;
    case Message::Join:
        //: Join Message
        t = tr("%DN%1%DN %DH(%2@%3)%DH has joined %DC%4%DC").arg(nick, user, host, bufferName);
        break;
    case Message::Part:
        //: Part Message
        t = tr("%DN%1%DN %DH(%2@%3)%DH has left %DC%4%DC").arg(nick, user, host, bufferName);
        if (!txt.isEmpty())
            t = QString("%1 (%2)").arg(t).arg(txt);
        break;
    case Message::Quit:
        //: Quit Message
        t = tr("%DN%1%DN %DH(%2@%3)%DH has quit").arg(nick, user, host);
        if (!txt.isEmpty())
            t = QString("%1 (%2)").arg(t).arg(txt);
        break;
    case Message::Kick: {
        QString victim = txt.section(" ", 0, 0);
        QString kickmsg = txt.section(" ", 1);
        //: Kick Message
        t = tr("%DN%1%DN has kicked %DN%2%DN from %DC%3%DC").arg(nick).arg(victim).arg(bufferName);
        if (!kickmsg.isEmpty())
            t = QString("%1 (%2)").arg(t).arg(kickmsg);
    } break;
        // case Message::Kill: FIXME

    case Message::Server:
        t = QString("%1").arg(txt);
        break;
    case Message::Info:
        t = QString("%1").arg(txt);
        break;
    case Message::Error:
        t = QString("%1").arg(txt);
        break;
    case Message::DayChange: {
        //: Day Change Message
        t = tr("{Day changed to %1}").arg(timestamp().date().toString(Qt::DefaultLocaleLongDate));
    } break;
    case Message::Topic:
        t = QString("%1").arg(txt);
        break;
    case Message::NetsplitJoin: {
        QStringList users = txt.split("#:#");
        QStringList servers = users.takeLast().split(" ");

        for (int i = 0; i < users.count() && i < maxNetsplitNicks; i++)
            users[i] = nickFromMask(users.at(i));

        t = tr("Netsplit between %DH%1%DH and %DH%2%DH ended. Users joined: ").arg(servers.at(0), servers.at(1));
        if (users.count() <= maxNetsplitNicks)
            t.append(QString("%DN%1%DN").arg(users.join(", ")));
        else
            t.append(tr("%DN%1%DN (%2 more)")
                         .arg(static_cast<QStringList>(users.mid(0, maxNetsplitNicks)).join(", "))
                         .arg(users.count() - maxNetsplitNicks));
    } break;
    case Message::NetsplitQuit: {
        QStringList users = txt.split("#:#");
        QStringList servers = users.takeLast().split(" ");

        for (int i = 0; i < users.count() && i < maxNetsplitNicks; i++)
            users[i] = nickFromMask(users.at(i));

        t = tr("Netsplit between %DH%1%DH and %DH%2%DH. Users quit: ").arg(servers.at(0), servers.at(1));

        if (users.count() <= maxNetsplitNicks)
            t.append(QString("%DN%1%DN").arg(users.join(", ")));
        else
            t.append(tr("%DN%1%DN (%2 more)")
                         .arg(static_cast<QStringList>(users.mid(0, maxNetsplitNicks)).join(", "))
                         .arg(users.count() - maxNetsplitNicks));
    } break;
    case Message::Invite:
        t = QString("%1").arg(txt);
        break;
    default:
        t = QString("[%1]").arg(txt);
    }
    _contents = UiStyle::styleString(t, UiStyle::formatType(type()));
}

const QString& UiStyle::StyledMessage::plainContents() const
{
    if (_contents.plainText.isNull())
        style();

    return _contents.plainText;
}

const UiStyle::FormatList& UiStyle::StyledMessage::contentsFormatList() const
{
    if (_contents.plainText.isNull())
        style();

    return _contents.formatList;
}

QString UiStyle::StyledMessage::decoratedTimestamp() const
{
    return timestamp().toLocalTime().toString(UiStyle::timestampFormatString());
}

QString UiStyle::StyledMessage::plainSender() const
{
    switch (type()) {
    case Message::Plain:
    case Message::Notice:
        return nickFromMask(sender());
    default:
        return QString();
    }
}

QString UiStyle::StyledMessage::decoratedSender() const
{
    QString _senderPrefixes;
    switch (_senderPrefixDisplay) {
    case UiStyle::SenderPrefixMode::AllModes:
        // Show every available mode
        _senderPrefixes = senderPrefixes();
        break;
    case UiStyle::SenderPrefixMode::HighestMode:
        // Show the highest available mode (left-most)
        _senderPrefixes = senderPrefixes().left(1);
        break;
    case UiStyle::SenderPrefixMode::NoModes:
        // Don't show any mode (already empty by default)
        break;
    }

    switch (type()) {
    case Message::Plain:
        if (_showSenderBrackets)
            return QString("<%1%2>").arg(_senderPrefixes, plainSender());
        else
            return QString("%1%2").arg(_senderPrefixes, plainSender());
    case Message::Notice:
        return QString("[%1%2]").arg(_senderPrefixes, plainSender());
    case Message::Action:
        return "-*-";
    case Message::Nick:
        return "<->";
    case Message::Mode:
        return "***";
    case Message::Join:
        return "-->";
    case Message::Part:
        return "<--";
    case Message::Quit:
        return "<--";
    case Message::Kick:
        return "<-*";
    case Message::Kill:
        return "<-x";
    case Message::Server:
        return "*";
    case Message::Info:
        return "*";
    case Message::Error:
        return "*";
    case Message::DayChange:
        return "-";
    case Message::Topic:
        return "*";
    case Message::NetsplitJoin:
        return "=>";
    case Message::NetsplitQuit:
        return "<=";
    case Message::Invite:
        return "->";
    }

    return QString("%1%2").arg(_senderPrefixes, plainSender());
}

// FIXME hardcoded to 16 sender hashes
quint8 UiStyle::StyledMessage::senderHash() const
{
    if (_senderHash != 0xff)
        return _senderHash;

    QString nick;

    // HACK: Until multiple nicknames with different colors can be solved in the theming engine,
    // for /nick change notifications, use the color of the new nickname (if possible), not the old
    // nickname.
    if (type() == Message::Nick) {
        // New nickname is given as contents.  Change to that.
        nick = stripFormatCodes(contents()).toLower();
    }
    else {
        // Just use the sender directly
        nick = nickFromMask(sender()).toLower();
    }

    if (!nick.isEmpty()) {
        int chopCount = 0;
        while (chopCount < nick.size() && nick.at(nick.count() - 1 - chopCount) == '_')
            chopCount++;
        if (chopCount < nick.size())
            nick.chop(chopCount);
    }


    // quint16 hash = qChecksum(nick.toLatin1().data(), nick.toLatin1().size());
    QByteArray nb = nick.toLocal8Bit();
    quint16 col = (qChecksum(nb, nb.size()) >> ((nb.length() + nb[0]) % 12)) & 0x0f;
    return (_senderHash = col + 1);
}

/***********************************************************************************/

uint qHash(UiStyle::ItemFormatType key, uint seed)
{
    return qHash(static_cast<quint32>(key), seed);
}

UiStyle::FormatType operator|(UiStyle::FormatType lhs, UiStyle::FormatType rhs)
{
    return static_cast<UiStyle::FormatType>(static_cast<quint32>(lhs) | static_cast<quint32>(rhs));
}

UiStyle::FormatType& operator|=(UiStyle::FormatType& lhs, UiStyle::FormatType rhs)
{
    lhs = static_cast<UiStyle::FormatType>(static_cast<quint32>(lhs) | static_cast<quint32>(rhs));
    return lhs;
}

UiStyle::FormatType operator|(UiStyle::FormatType lhs, quint32 rhs)
{
    return static_cast<UiStyle::FormatType>(static_cast<quint32>(lhs) | rhs);
}

UiStyle::FormatType& operator|=(UiStyle::FormatType& lhs, quint32 rhs)
{
    lhs = static_cast<UiStyle::FormatType>(static_cast<quint32>(lhs) | rhs);
    return lhs;
}

UiStyle::FormatType operator&(UiStyle::FormatType lhs, UiStyle::FormatType rhs)
{
    return static_cast<UiStyle::FormatType>(static_cast<quint32>(lhs) & static_cast<quint32>(rhs));
}

UiStyle::FormatType& operator&=(UiStyle::FormatType& lhs, UiStyle::FormatType rhs)
{
    lhs = static_cast<UiStyle::FormatType>(static_cast<quint32>(lhs) & static_cast<quint32>(rhs));
    return lhs;
}

UiStyle::FormatType operator&(UiStyle::FormatType lhs, quint32 rhs)
{
    return static_cast<UiStyle::FormatType>(static_cast<quint32>(lhs) & rhs);
}

UiStyle::FormatType& operator&=(UiStyle::FormatType& lhs, quint32 rhs)
{
    lhs = static_cast<UiStyle::FormatType>(static_cast<quint32>(lhs) & rhs);
    return lhs;
}

UiStyle::FormatType& operator^=(UiStyle::FormatType& lhs, UiStyle::FormatType rhs)
{
    lhs = static_cast<UiStyle::FormatType>(static_cast<quint32>(lhs) ^ static_cast<quint32>(rhs));
    return lhs;
}

UiStyle::MessageLabel operator|(UiStyle::MessageLabel lhs, UiStyle::MessageLabel rhs)
{
    return static_cast<UiStyle::MessageLabel>(static_cast<quint32>(lhs) | static_cast<quint32>(rhs));
}

UiStyle::MessageLabel& operator|=(UiStyle::MessageLabel& lhs, UiStyle::MessageLabel rhs)
{
    lhs = static_cast<UiStyle::MessageLabel>(static_cast<quint32>(lhs) | static_cast<quint32>(rhs));
    return lhs;
}

UiStyle::MessageLabel operator&(UiStyle::MessageLabel lhs, quint32 rhs)
{
    return static_cast<UiStyle::MessageLabel>(static_cast<quint32>(lhs) & rhs);
}

UiStyle::MessageLabel& operator&=(UiStyle::MessageLabel& lhs, UiStyle::MessageLabel rhs)
{
    lhs = static_cast<UiStyle::MessageLabel>(static_cast<quint32>(lhs) & static_cast<quint32>(rhs));
    return lhs;
}

quint64 operator|(UiStyle::FormatType lhs, UiStyle::MessageLabel rhs)
{
    return static_cast<quint64>(lhs) | (static_cast<quint64>(rhs) << 32ull);
}

UiStyle::ItemFormatType operator|(UiStyle::ItemFormatType lhs, UiStyle::ItemFormatType rhs)
{
    return static_cast<UiStyle::ItemFormatType>(static_cast<quint32>(lhs) | static_cast<quint32>(rhs));
}

UiStyle::ItemFormatType& operator|=(UiStyle::ItemFormatType& lhs, UiStyle::ItemFormatType rhs)
{
    lhs = static_cast<UiStyle::ItemFormatType>(static_cast<quint32>(lhs) | static_cast<quint32>(rhs));
    return lhs;
}

/***********************************************************************************/

QDataStream& operator<<(QDataStream& out, const UiStyle::FormatList& formatList)
{
    out << static_cast<quint16>(formatList.size());
    auto it = formatList.cbegin();
    while (it != formatList.cend()) {
        out << it->first << static_cast<quint32>(it->second.type) << it->second.foreground << it->second.background;
        ++it;
    }
    return out;
}

QDataStream& operator>>(QDataStream& in, UiStyle::FormatList& formatList)
{
    quint16 cnt;
    in >> cnt;
    for (quint16 i = 0; i < cnt; i++) {
        quint16 pos;
        quint32 ftype;
        QColor foreground;
        QColor background;
        in >> pos >> ftype >> foreground >> background;
        formatList.emplace_back(
            std::make_pair(quint16{pos}, UiStyle::Format{static_cast<UiStyle::FormatType>(ftype), foreground, background}));
    }
    return in;
}
