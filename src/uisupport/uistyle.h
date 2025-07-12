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

#pragma once

#include "uisupport-export.h"

#include <utility>
#include <vector>

#include <QColor>
#include <QDataStream>
#include <QFontMetricsF>
#include <QHash>
#include <QIcon>
#include <QMetaType>
#include <QPalette>
#include <QTextCharFormat>
#include <QTextLayout>
#include <QVector>

#include "bufferinfo.h"
#include "message.h"
#include "networkmodel.h"
#include "settings.h"

class UISUPPORT_EXPORT UiStyle : public QObject
{
    Q_OBJECT
    Q_ENUMS(SenderPrefixMode)

public:
    UiStyle(QObject* parent = nullptr);
    ~UiStyle() override;

#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    using FormatContainer = QVector<QTextLayout::FormatRange>;
    static inline void setTextLayoutFormats(QTextLayout& layout, const FormatContainer& formats) { layout.setFormats(formats); }
    static inline QVector<QTextLayout::FormatRange> containerToVector(const FormatContainer& container) { return container; }
#else
    using FormatContainer = QList<QTextLayout::FormatRange>;
    static inline void setTextLayoutFormats(QTextLayout& layout, const FormatContainer& formats) { layout.setAdditionalFormats(formats); }
    static inline QVector<QTextLayout::FormatRange> containerToVector(const FormatContainer& container) { return container.toVector(); }
#endif

    enum class FormatType : quint32
    {
        Base = 0x00000000,
        Invalid = 0xffffffff,
        PlainMsg = 0x00000001,
        NoticeMsg = 0x00000002,
        ActionMsg = 0x00000003,
        NickMsg = 0x00000004,
        ModeMsg = 0x00000005,
        JoinMsg = 0x00000006,
        PartMsg = 0x00000007,
        QuitMsg = 0x00000008,
        KickMsg = 0x00000009,
        KillMsg = 0x0000000a,
        ServerMsg = 0x0000000b,
        InfoMsg = 0x0000000c,
        ErrorMsg = 0x0000000d,
        DayChangeMsg = 0x0000000e,
        TopicMsg = 0x0000000f,
        NetsplitJoinMsg = 0x00000010,
        NetsplitQuitMsg = 0x00000020,
        InviteMsg = 0x00000030,
        Bold = 0x00000100,
        Italic = 0x00000200,
        Underline = 0x00000400,
        Strikethrough = 0x00000800,
        Timestamp = 0x00001000,
        Sender = 0x00002000,
        Contents = 0x00004000,
        Nick = 0x00008000,
        Hostmask = 0x00010000,
        ChannelName = 0x00020000,
        ModeFlags = 0x00040000,
        Url = 0x00080000
    };

    enum class MessageLabel : quint32
    {
        None = 0x00000000,
        OwnMsg = 0x00000001,
        Highlight = 0x00000002,
        Selected = 0x00000004,
        Hovered = 0x00000008,
        Last = Hovered
    };

    enum class ItemFormatType : quint32
    {
        None = 0x00000000,
        BufferViewItem = 0x00000001,
        NickViewItem = 0x00000002,
        NetworkItem = 0x00000010,
        ChannelBufferItem = 0x00000020,
        QueryBufferItem = 0x00000040,
        IrcUserItem = 0x00000080,
        UserCategoryItem = 0x00000100,
        InactiveBuffer = 0x00001000,
        ActiveBuffer = 0x00002000,
        UnreadBuffer = 0x00004000,
        HighlightedBuffer = 0x00008000,
        UserAway = 0x00010000,
        Invalid = 0xffffffff
    };

    enum class FormatProperty
    {
        AllowForegroundOverride = QTextFormat::UserProperty,
        AllowBackgroundOverride
    };

    enum class ColorRole
    {
        MarkerLine,
        SenderColorSelf,
        SenderColor00,
        SenderColor01,
        SenderColor02,
        SenderColor03,
        SenderColor04,
        SenderColor05,
        SenderColor06,
        SenderColor07,
        SenderColor08,
        SenderColor09,
        SenderColor0a,
        SenderColor0b,
        SenderColor0c,
        SenderColor0d,
        SenderColor0e,
        SenderColor0f,
        NumRoles
    };

    enum class SenderPrefixMode
    {
        NoModes = 0,
        HighestMode = 1,
        AllModes = 2
    };

    struct Format
    {
        FormatType type;
        QColor foreground;
        QColor background;
    };

    using FormatList = std::vector<std::pair<quint16, Format>>;

    struct StyledString
    {
        QString plainText;
        FormatList formatList;
    };

    class StyledMessage;

    const QList<QColor> defaultSenderColors = QList<QColor>{QColor(204, 0, 0),
                                                            QColor(0, 108, 173),
                                                            QColor(77, 153, 0),
                                                            QColor(102, 0, 204),
                                                            QColor(166, 125, 0),
                                                            QColor(0, 153, 39),
                                                            QColor(0, 48, 192),
                                                            QColor(204, 0, 154),
                                                            QColor(185, 70, 0),
                                                            QColor(134, 153, 0),
                                                            QColor(20, 153, 0),
                                                            QColor(0, 153, 96),
                                                            QColor(0, 108, 173),
                                                            QColor(0, 153, 204),
                                                            QColor(179, 0, 204),
                                                            QColor(204, 0, 77)};

    const QColor defaultSenderColorSelf = QColor(0, 0, 0);

    static FormatType formatType(Message::Type msgType);
    static StyledString styleString(const QString& string, FormatType baseFormat = FormatType::Base);
    static QString mircToInternal(const QString&);

    static inline bool useCustomTimestampFormat() { return _useCustomTimestampFormat; }
    static QString systemTimestampFormatString();
    static QString timestampFormatString();

    QTextCharFormat format(const Format& format, MessageLabel messageLabel) const;
    QFontMetricsF* fontMetrics(FormatType formatType, MessageLabel messageLabel) const;

    FormatContainer toTextLayoutList(const FormatList&, int textLength, MessageLabel messageLabel) const;

    inline const QBrush& brush(ColorRole role) const { return _uiStylePalette.at((int)role); }
    inline void setBrush(ColorRole role, const QBrush& brush) { _uiStylePalette[(int)role] = brush; }

    QVariant bufferViewItemData(const QModelIndex& networkModelIndex, int role) const;
    QVariant nickViewItemData(const QModelIndex& networkModelIndex, int role) const;

public slots:
    void reload();

signals:
    void changed();

protected:
    void loadStyleSheet();
    QString loadStyleSheet(const QString& name, bool shouldExist = false);

    QTextCharFormat parsedFormat(quint64 key) const;
    QTextCharFormat cachedFormat(const Format& format, MessageLabel messageLabel) const;
    void setCachedFormat(const QTextCharFormat& charFormat, const Format& format, MessageLabel messageLabel) const;
    void mergeFormat(QTextCharFormat& charFormat, const Format& format, MessageLabel messageLabel) const;
    void mergeSubElementFormat(QTextCharFormat& charFormat, FormatType formatType, MessageLabel messageLabel) const;
    void mergeColors(QTextCharFormat& charFormat, const Format& format, MessageLabel messageLabel) const;

    static FormatType formatType(const QString& code);
    static QString formatCode(FormatType);

    static void updateSystemTimestampFormat();
    static void setUseCustomTimestampFormat(bool enabled);
    static void setTimestampFormatString(const QString& format);
    static void setSenderPrefixDisplay(UiStyle::SenderPrefixMode mode);
    static void enableSenderBrackets(bool enabled);

    QVariant itemData(int role, const QTextCharFormat& format) const;

private slots:
    void allowMircColorsChanged(const QVariant&);
    void showItemViewIconsChanged(const QVariant&);

private:
    QVector<QBrush> _uiStylePalette;
    QBrush _markerLineBrush;
    QHash<quint64, QTextCharFormat> _formats;
    mutable QHash<QString, QTextCharFormat> _formatCache;
    mutable QHash<quint64, QFontMetricsF*> _metricsCache;
    QHash<UiStyle::ItemFormatType, QTextCharFormat> _listItemFormats;
    static QHash<QString, FormatType> _formatCodes;
    static bool _useCustomTimestampFormat;
    static QString _systemTimestampFormatString;
    static QString _timestampFormatString;
    static UiStyle::SenderPrefixMode _senderPrefixDisplay;
    static bool _showSenderBrackets;

    QIcon _channelJoinedIcon;
    QIcon _channelPartedIcon;
    QIcon _userOfflineIcon;
    QIcon _userOnlineIcon;
    QIcon _userAwayIcon;
    QIcon _categoryOpIcon;
    QIcon _categoryVoiceIcon;
    int _opIconLimit;
    int _voiceIconLimit;
    bool _showNickViewIcons;
    bool _showBufferViewIcons;
    bool _allowMircColors;
};

class UISUPPORT_EXPORT UiStyle::StyledMessage : public Message
{
    Q_DECLARE_TR_FUNCTIONS(UiStyle::StyledMessage)

public:
    explicit StyledMessage(const Message& message);

    QString decoratedTimestamp() const;
    QString plainSender() const;
    QString decoratedSender() const;
    const QString& plainContents() const;
    const FormatList& contentsFormatList() const;
    quint8 senderHash() const;

protected:
    void style() const;

private:
    mutable StyledString _contents;
    mutable quint8 _senderHash;
};

// Streaming operators for Format
inline QDataStream& operator<<(QDataStream& out, const UiStyle::Format& format)
{
    out << static_cast<quint32>(format.type) << format.foreground << format.background;
    return out;
}

inline QDataStream& operator>>(QDataStream& in, UiStyle::Format& format)
{
    quint32 type;
    in >> type >> format.foreground >> format.background;
    format.type = static_cast<UiStyle::FormatType>(type);
    return in;
}

// Streaming operators for MessageLabel
inline QDataStream& operator<<(QDataStream& out, const UiStyle::MessageLabel& label)
{
    out << static_cast<quint32>(label);
    return out;
}

inline QDataStream& operator>>(QDataStream& in, UiStyle::MessageLabel& label)
{
    quint32 value;
    in >> value;
    label = static_cast<UiStyle::MessageLabel>(value);
    return in;
}

// Streaming operators for SenderPrefixMode
inline QDataStream& operator<<(QDataStream& out, const UiStyle::SenderPrefixMode& mode)
{
    out << static_cast<quint32>(mode);
    return out;
}

inline QDataStream& operator>>(QDataStream& in, UiStyle::SenderPrefixMode& mode)
{
    quint32 value;
    in >> value;
    mode = static_cast<UiStyle::SenderPrefixMode>(value);
    return in;
}

uint qHash(UiStyle::ItemFormatType key, uint seed);

UISUPPORT_EXPORT UiStyle::FormatType operator|(UiStyle::FormatType lhs, UiStyle::FormatType rhs);
UISUPPORT_EXPORT UiStyle::FormatType& operator|=(UiStyle::FormatType& lhs, UiStyle::FormatType rhs);
UISUPPORT_EXPORT UiStyle::FormatType operator|(UiStyle::FormatType lhs, quint32 rhs);
UISUPPORT_EXPORT UiStyle::FormatType& operator|=(UiStyle::FormatType& lhs, quint32 rhs);
UISUPPORT_EXPORT UiStyle::FormatType operator&(UiStyle::FormatType lhs, UiStyle::FormatType rhs);
UISUPPORT_EXPORT UiStyle::FormatType& operator&=(UiStyle::FormatType& lhs, UiStyle::FormatType rhs);
UISUPPORT_EXPORT UiStyle::FormatType operator&(UiStyle::FormatType lhs, quint32 rhs);
UISUPPORT_EXPORT UiStyle::FormatType& operator&=(UiStyle::FormatType& lhs, quint32 rhs);
UISUPPORT_EXPORT UiStyle::FormatType& operator^=(UiStyle::FormatType& lhs, UiStyle::FormatType rhs);

UISUPPORT_EXPORT UiStyle::MessageLabel operator|(UiStyle::MessageLabel lhs, UiStyle::MessageLabel rhs);
UISUPPORT_EXPORT UiStyle::MessageLabel& operator|=(UiStyle::MessageLabel& lhs, UiStyle::MessageLabel rhs);
UISUPPORT_EXPORT UiStyle::MessageLabel operator&(UiStyle::MessageLabel lhs, quint32 rhs);
UISUPPORT_EXPORT UiStyle::MessageLabel& operator&=(UiStyle::MessageLabel& lhs, UiStyle::MessageLabel rhs);

UISUPPORT_EXPORT quint64 operator|(UiStyle::FormatType lhs, UiStyle::MessageLabel rhs);

UISUPPORT_EXPORT UiStyle::ItemFormatType operator|(UiStyle::ItemFormatType lhs, UiStyle::ItemFormatType rhs);
UISUPPORT_EXPORT UiStyle::ItemFormatType& operator|=(UiStyle::ItemFormatType& lhs, UiStyle::ItemFormatType rhs);

QDataStream& operator<<(QDataStream& out, const UiStyle::FormatList& formatList);
QDataStream& operator>>(QDataStream& in, UiStyle::FormatList& formatList);

Q_DECLARE_METATYPE(UiStyle::Format)
Q_DECLARE_METATYPE(UiStyle::FormatList)
Q_DECLARE_METATYPE(UiStyle::MessageLabel)
Q_DECLARE_METATYPE(UiStyle::SenderPrefixMode)
