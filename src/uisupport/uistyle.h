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

#ifndef UISTYLE_H_
#define UISTYLE_H_

#include <QDataStream>
#include <QFontMetricsF>
#include <QHash>
#include <QTextCharFormat>
#include <QTextLayout>
#include <QPalette>
#include <QVector>

#include "message.h"
#include "settings.h"

class UiStyle {
  Q_DECLARE_TR_FUNCTIONS(UiStyle)

public:
  UiStyle();
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
    ActionMsg       = 0x00000003,
    NickMsg         = 0x00000004,
    ModeMsg         = 0x00000005,
    JoinMsg         = 0x00000006,
    PartMsg         = 0x00000007,
    QuitMsg         = 0x00000008,
    KickMsg         = 0x00000009,
    KillMsg         = 0x0000000a,
    ServerMsg       = 0x0000000b,
    InfoMsg         = 0x0000000c,
    ErrorMsg        = 0x0000000d,
    DayChangeMsg    = 0x0000000e,

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
    Url             = 0x00080000

    // mIRC Colors - we assume those to be present only in plain contents
    // foreground: 0x0.400000
    // background: 0x.0800000
  };

  enum MessageLabel {
    OwnMsg          = 0x00000001,
    Highlight       = 0x00000002
  };

  struct StyledString {
    QString plainText;
    FormatList formatList;  // starting pos, ftypes
  };

  class StyledMessage;

  StyledString styleString(const QString &string, quint32 baseFormat = None);
  QString mircToInternal(const QString &) const;

  FormatType formatType(Message::Type msgType) const;

  QTextCharFormat format(quint32 formatType, quint32 messageLabel = 0);
  QFontMetricsF *fontMetrics(quint32 formatType, quint32 messageLabel = 0);

  inline QFont defaultFont() const { return _defaultFont; }

  QList<QTextLayout::FormatRange> toTextLayoutList(const FormatList &, int textLength);

protected:
  void loadStyleSheet();

  //! Determines the format set to be used for the given hostmask
  //int formatSetIndex(const QString &hostmask) const;
  //int formatSetIndexForSelf() const;

  QTextCharFormat cachedFormat(quint64 key) const;
  QTextCharFormat cachedFormat(quint32 formatType, quint32 messageLabel = 0) const;
  void setCachedFormat(const QTextCharFormat &format, quint32 formatType, quint32 messageLabel = 0);
  void mergeSubElementFormat(QTextCharFormat &format, quint32 formatType, quint32 messageLabel = 0);

  FormatType formatType(const QString &code) const;
  QString formatCode(FormatType) const;

private:
  QFont _defaultFont;
  QHash<quint64, QTextCharFormat> _formatCache;
  QHash<quint64, QFontMetricsF *> _metricsCache;
  QHash<QString, FormatType> _formatCodes;
};

class UiStyle::StyledMessage : public Message {
public:
  explicit StyledMessage(const Message &message);

  //! Styling is only needed for calls to plainContents() and contentsFormatList()
  // StyledMessage can't style lazily by itself, as it doesn't know the used style
  bool inline needsStyling() const { return _contents.plainText.isNull(); }
  void style(UiStyle *style) const;


  QString decoratedTimestamp() const;
  QString plainSender() const;             //!< Nickname (no decorations) for Plain and Notice, empty else
  QString decoratedSender() const;
  inline const QString &plainContents() const { return _contents.plainText; }

  inline FormatType timestampFormat() const { return UiStyle::Timestamp; }
  FormatType senderFormat() const;
  inline const FormatList &contentsFormatList() const { return _contents.formatList; }

private:
  mutable StyledString _contents;
};

QDataStream &operator<<(QDataStream &out, const UiStyle::FormatList &formatList);
QDataStream &operator>>(QDataStream &in, UiStyle::FormatList &formatList);

Q_DECLARE_METATYPE(UiStyle::FormatList)

#endif
