/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel Project                          *
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

#ifndef _UISTYLE_H_
#define _UISTYLE_H_

#include <QDataStream>
#include <QFontMetricsF>
#include <QTextCharFormat>
#include <QTextLayout>
#include <QUrl>

#include "message.h"
#include "settings.h"

class UiStyle {
  Q_DECLARE_TR_FUNCTIONS(UiStyle)

  public:
    UiStyle(const QString &settingsKey);
    virtual ~UiStyle();

    typedef QList<QPair<quint16, quint32> > FormatList;

    //! This enumerates the possible formats a text element may have. */
    /** These formats are ordered on increasing importance, in cases where a given property is specified
     *  by multiple active formats.
     *  \NOTE: Do not change/add values here without also adapting the relevant
     *         methods in this class (in particular mergedFormat())!
     *         Also, we _do_ rely on certain properties of these values in styleString() and friends!
     */
    enum FormatType {
      None            = 0x00000000,
      Invalid         = 0x11111111,
      // Message Formats (mutually exclusive!)
      PlainMsg        = 0x00000001,
      NoticeMsg       = 0x00000002,
      ServerMsg       = 0x00000003,
      ErrorMsg        = 0x00000004,
      JoinMsg         = 0x00000005,
      PartMsg         = 0x00000006,
      QuitMsg         = 0x00000007,
      KickMsg         = 0x00000008,
      RenameMsg       = 0x00000009,
      ModeMsg         = 0x0000000a,
      ActionMsg       = 0x0000000b,
      // Standard Formats
      Bold            = 0x00000010,
      Italic          = 0x00000020,
      Underline       = 0x00000040,
      Reverse         = 0x00000080,
      // Individual parts of a message
      Timestamp       = 0x00000100,
      Sender          = 0x00000200,
      Nick            = 0x00000400,
      Hostmask        = 0x00000800,
      ChannelName     = 0x00001000,
      ModeFlags       = 0x00002000,
      // URL is special, we want that to take precedence over the rest...
      Url             = 0x00100000,
      // Colors
      FgCol00         = 0x00400000,
      FgCol01         = 0x01400000,
      FgCol02         = 0x02400000,
      FgCol03         = 0x03400000,
      FgCol04         = 0x04400000,
      FgCol05         = 0x05400000,
      FgCol06         = 0x06400000,
      FgCol07         = 0x07400000,
      FgCol08         = 0x08400000,
      FgCol09         = 0x09400000,
      FgCol10         = 0x0a400000,
      FgCol11         = 0x0b400000,
      FgCol12         = 0x0c400000,
      FgCol13         = 0x0d400000,
      FgCol14         = 0x0e400000,
      FgCol15         = 0x0f400000,

      BgCol00         = 0x00800000,
      BgCol01         = 0x10800000,
      BgCol02         = 0x20800000,
      BgCol03         = 0x30800000,
      BgCol04         = 0x40800000,
      BgCol05         = 0x50800000,
      BgCol06         = 0x60800000,
      BgCol07         = 0x70800000,
      BgCol08         = 0x80800000,
      BgCol09         = 0x90800000,
      BgCol10         = 0xa0800000,
      BgCol11         = 0xb0800000,
      BgCol12         = 0xc0800000,
      BgCol13         = 0xd0800000,
      BgCol14         = 0xe0800000,
      BgCol15         = 0xf0800000

    };

    struct UrlInfo {
      int start, end;
      QUrl url;
    };

    struct StyledString {
      QString plainText;
      FormatList formatList;  // starting pos, ftypes
    };

    class StyledMessage;

    StyledString styleString(const QString &);
    StyledMessage styleMessage(const Message &);
    QString mircToInternal(const QString &) const;

    void setFormat(FormatType, QTextCharFormat, Settings::Mode mode/* = Settings::Custom*/);
    QTextCharFormat format(FormatType, Settings::Mode mode = Settings::Custom) const;
    QTextCharFormat mergedFormat(quint32 formatType);
    QFontMetricsF *fontMetrics(quint32 formatType);

    FormatType formatType(const QString &code) const;
    QString formatCode(FormatType) const;

    inline QFont defaultFont() const { return _defaultFont; }

    QList<QTextLayout::FormatRange> toTextLayoutList(const FormatList &, int textLength);

  protected:


  private:

    QFont _defaultFont;
    QTextCharFormat _defaultPlainFormat;
    QHash<FormatType, QTextCharFormat> _defaultFormats;
    QHash<FormatType, QTextCharFormat> _customFormats;
    QHash<quint32, QTextCharFormat> _cachedFormats;
    QHash<quint32, QFontMetricsF *> _cachedFontMetrics;
    QHash<QString, FormatType> _formatCodes;

    QString _settingsKey;
};

class UiStyle::StyledMessage {

  public:
    explicit StyledMessage(const Message &, UiStyle *style);

    QDateTime timestamp() const;
    QString decoratedTimestamp() const;
    QString sender() const;             //!< Nickname (no decorations) for Plain and Notice, empty else
    QString decoratedSender() const;
    QString contents() const;

    FormatType timestampFormat() const;
    FormatType senderFormat() const;
    FormatList contentsFormatList() const;

    inline Message::Type type() const { return _msgType; }

  private:
    StyledString _contents;
    QDateTime _timestamp;
    QString _sender;
    Message::Type _msgType;
};

QDataStream &operator<<(QDataStream &out, const UiStyle::FormatList &formatList);
QDataStream &operator>>(QDataStream &in, UiStyle::FormatList &formatList);

Q_DECLARE_METATYPE(UiStyle::FormatList)

#endif
