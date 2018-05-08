/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
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

#include <QFontMetrics>
#include <QTextBoundaryFinder>

#include "chatlinemodelitem.h"
#include "chatlinemodel.h"
#include "qtui.h"
#include "qtuistyle.h"

// This Struct is taken from Harfbuzz. We use it only to calc it's size.
// we use a shared memory region so we do not have to malloc a buffer area for every line
typedef struct {
    /*HB_LineBreakType*/ unsigned lineBreakType  : 2;
    /*HB_Bool*/ unsigned whiteSpace              : 1;     /* A unicode whitespace character, except NBSP, ZWNBSP */
    /*HB_Bool*/ unsigned charStop                : 1;     /* Valid cursor position (for left/right arrow) */
    /*HB_Bool*/ unsigned wordBoundary            : 1;
    /*HB_Bool*/ unsigned sentenceBoundary        : 1;
    unsigned unused                  : 2;
} HB_CharAttributes_Dummy;

unsigned char *ChatLineModelItem::TextBoundaryFinderBuffer = (unsigned char *)malloc(512 * sizeof(HB_CharAttributes_Dummy));
int ChatLineModelItem::TextBoundaryFinderBufferSize = 512 * (sizeof(HB_CharAttributes_Dummy) / sizeof(unsigned char));

// ****************************************
// the actual ChatLineModelItem
// ****************************************
ChatLineModelItem::ChatLineModelItem(const Message &msg)
    : MessageModelItem(),
    _styledMsg(msg)
{
    if (!msg.sender().contains('!'))
        _styledMsg.setFlags(msg.flags() |= Message::ServerMsg);

    if (_styledMsg.type() == Message::Nick) {
        // HACK: Work around nick changes on Quassel core not properly being set as Self
        // While this is fixed in the core, old cores and past history will still show incorrectly.
        if (nickFromMask(_styledMsg.sender()) == stripFormatCodes(_styledMsg.contents()).toLower()) {
            _styledMsg.setFlags(msg.flags() |= Message::Self);
        }
    }
    // Unfortunately, the missing Self flag for other message types can't easily be worked around.
}


bool ChatLineModelItem::setData(int column, const QVariant &value, int role)
{
    switch (role) {
    case MessageModel::FlagsRole:
        _styledMsg.setFlags((Message::Flags)value.toInt());
        return true;
    default:
        return MessageModelItem::setData(column, value, role);
    }
}


QVariant ChatLineModelItem::data(int column, int role) const
{
    if (role == ChatLineModel::MsgLabelRole)
        return QVariant::fromValue<UiStyle::MessageLabel>(messageLabel());

    QVariant variant;
    MessageModel::ColumnType col = (MessageModel::ColumnType)column;
    switch (col) {
    case ChatLineModel::TimestampColumn:
        variant = timestampData(role);
        break;
    case ChatLineModel::SenderColumn:
        variant = senderData(role);
        break;
    case ChatLineModel::ContentsColumn:
        variant = contentsData(role);
        break;
    default:
        break;
    }
    if (!variant.isValid())
        return MessageModelItem::data(column, role);
    return variant;
}


QVariant ChatLineModelItem::timestampData(int role) const
{
    switch (role) {
    case ChatLineModel::DisplayRole:
        return _styledMsg.decoratedTimestamp();
    case ChatLineModel::EditRole:
        return _styledMsg.timestamp();
    case ChatLineModel::BackgroundRole:
        return backgroundBrush(UiStyle::FormatType::Timestamp);
    case ChatLineModel::SelectedBackgroundRole:
        return backgroundBrush(UiStyle::FormatType::Timestamp, true);
    case ChatLineModel::FormatRole:
        return QVariant::fromValue<UiStyle::FormatList>({std::make_pair(quint16{0}, UiStyle::Format{UiStyle::formatType(_styledMsg.type()) | UiStyle::FormatType::Timestamp, {}, {}})});
    }
    return QVariant();
}


QVariant ChatLineModelItem::senderData(int role) const
{
    switch (role) {
    case ChatLineModel::DisplayRole:
        return _styledMsg.decoratedSender();
    case ChatLineModel::EditRole:
        return _styledMsg.plainSender();
    case ChatLineModel::BackgroundRole:
        return backgroundBrush(UiStyle::FormatType::Sender);
    case ChatLineModel::SelectedBackgroundRole:
        return backgroundBrush(UiStyle::FormatType::Sender, true);
    case ChatLineModel::FormatRole:
        return QVariant::fromValue<UiStyle::FormatList>({std::make_pair(quint16{0}, UiStyle::Format{UiStyle::formatType(_styledMsg.type()) | UiStyle::FormatType::Sender, {}, {}})});
    }
    return QVariant();
}


QVariant ChatLineModelItem::contentsData(int role) const
{
    switch (role) {
    case ChatLineModel::DisplayRole:
    case ChatLineModel::EditRole:
        return _styledMsg.plainContents();
    case ChatLineModel::BackgroundRole:
        return backgroundBrush(UiStyle::FormatType::Contents);
    case ChatLineModel::SelectedBackgroundRole:
        return backgroundBrush(UiStyle::FormatType::Contents, true);
    case ChatLineModel::FormatRole:
        return QVariant::fromValue<UiStyle::FormatList>(_styledMsg.contentsFormatList());
    case ChatLineModel::WrapListRole:
        if (_wrapList.isEmpty())
            computeWrapList();
        return QVariant::fromValue<ChatLineModel::WrapList>(_wrapList);
    }
    return QVariant();
}


UiStyle::MessageLabel ChatLineModelItem::messageLabel() const
{
    using MessageLabel = UiStyle::MessageLabel;

    MessageLabel label = static_cast<MessageLabel>(_styledMsg.senderHash() << 16);
    if (_styledMsg.flags() & Message::Self)
        label |= MessageLabel::OwnMsg;
    if (_styledMsg.flags() & Message::Highlight)
        label |= MessageLabel::Highlight;
    return label;
}


QVariant ChatLineModelItem::backgroundBrush(UiStyle::FormatType subelement, bool selected) const
{
    QTextCharFormat fmt = QtUi::style()->format({UiStyle::formatType(_styledMsg.type()) | subelement, {}, {}},
                                                messageLabel() | (selected ? UiStyle::MessageLabel::Selected : UiStyle::MessageLabel::None));
    if (fmt.hasProperty(QTextFormat::BackgroundBrush))
        return QVariant::fromValue<QBrush>(fmt.background());
    return QVariant();
}


void ChatLineModelItem::computeWrapList() const
{
    QString text = _styledMsg.plainContents();
    int length = text.length();
    if (!length)
        return;

    QList<ChatLineModel::Word> wplist; // use a temp list which we'll later copy into a QVector for efficiency
    QTextBoundaryFinder finder(QTextBoundaryFinder::Line, _styledMsg.plainContents().unicode(), length,
        TextBoundaryFinderBuffer, TextBoundaryFinderBufferSize);

    int idx;
    int oldidx = 0;
    ChatLineModel::Word word;
    word.start = 0;
    qreal wordstartx = 0;

    QTextLayout layout(_styledMsg.plainContents());
    QTextOption option;
    option.setWrapMode(QTextOption::NoWrap);
    layout.setTextOption(option);

    layout.setAdditionalFormats(QtUi::style()->toTextLayoutList(_styledMsg.contentsFormatList(), length, messageLabel()));
    layout.beginLayout();
    QTextLine line = layout.createLine();
    line.setNumColumns(length);
    layout.endLayout();

    while ((idx = finder.toNextBoundary()) >= 0 && idx <= length) {
        // QTextBoundaryFinder has inconsistent behavior in Qt version up to and including 4.6.3 (at least).
        // It doesn't point to the position we should break, but to the character before that.
        // Unfortunately Qt decided to fix this by changing the behavior of QTBF, so now we have to add a version
        // check. At the time of this writing, I'm still trying to get this reverted upstream...
        //
        // cf. https://bugs.webkit.org/show_bug.cgi?id=31076 and Qt commit e6ac173
        static int needWorkaround = -1;
        if (needWorkaround < 0) {
            needWorkaround = 0;
            QStringList versions = QString(qVersion()).split('.');
            if (versions.count() == 3 && versions.at(0).toInt() == 4) {
                if (versions.at(1).toInt() <= 6 && versions.at(2).toInt() <= 3)
                    needWorkaround = 1;
            }
        }
        if (needWorkaround == 1) {
            if (idx < length)
                idx++;
        }

        if (idx == oldidx)
            continue;

        word.start = oldidx;
        int wordend = idx;
        for (; wordend > word.start; wordend--) {
            if (!text.at(wordend-1).isSpace())
                break;
        }

        qreal wordendx = line.cursorToX(wordend);
        qreal trailingendx = line.cursorToX(idx);
        word.endX = wordendx;
        word.width = wordendx - wordstartx;
        word.trailing = trailingendx - wordendx;
        wordstartx = trailingendx;
        wplist.append(word);

        oldidx = idx;
    }

    // A QVector needs less space than a QList
    _wrapList.resize(wplist.count());
    for (int i = 0; i < wplist.count(); i++) {
        _wrapList[i] = wplist.at(i);
    }
}
