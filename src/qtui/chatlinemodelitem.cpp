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

#include <QFontMetrics>
#include <QTextBoundaryFinder>

#include "chatlinemodelitem.h"
#include "chatlinemodel.h"
#include "qtui.h"
#include "qtuistyle.h"

// This Struct is taken from Harfbuzz. We use it only to calc it's size.
// we use a shared memory region so we do not have to malloc a buffer area for every line
typedef struct {
    /*HB_LineBreakType*/ unsigned lineBreakType  :2;
    /*HB_Bool*/ unsigned whiteSpace              :1;     /* A unicode whitespace character, except NBSP, ZWNBSP */
    /*HB_Bool*/ unsigned charStop                :1;     /* Valid cursor position (for left/right arrow) */
    /*HB_Bool*/ unsigned wordBoundary            :1;
    /*HB_Bool*/ unsigned sentenceBoundary        :1;
    unsigned unused                  :2;
} HB_CharAttributes_Dummy;

// PRIVATE DATA FOR CHATLINE MODEL ITEM
class ChatLineModelItemPrivate {
  struct ChatLinePart {
    QString plainText;
    UiStyle::FormatList formatList;
    inline ChatLinePart(const QString &pT, const UiStyle::FormatList &fL) : plainText(pT), formatList(fL) {}
  };

public:
  inline ChatLineModelItemPrivate(const Message &msg) : _msgBuffer(new Message(msg)), timestamp(0), sender(0), contents(0) {}
  inline ~ChatLineModelItemPrivate() {
    if(_msgBuffer) {
      delete _msgBuffer;
    } else {
      delete timestamp;
      delete sender;
      delete contents;
    }
  }

  inline bool needsStyling() { return (bool)_msgBuffer; }

  inline ChatLinePart *partByColumn(MessageModel::ColumnType column) {
    switch(column) {
    case ChatLineModel::TimestampColumn:
      return timestamp;
    case ChatLineModel::SenderColumn:
      return sender;
    case ChatLineModel::ContentsColumn:
      return contents;
    default:
      Q_ASSERT(false);
      return 0;
    }
  }

  inline const QString &plainText(MessageModel::ColumnType column) {
    if(needsStyling())
      style();
    return partByColumn(column)->plainText;
  }

  inline const UiStyle::FormatList &formatList(MessageModel::ColumnType column) {
    if(needsStyling())
      style();
    return partByColumn(column)->formatList;
  }

  inline const ChatLineModel::WrapList &wrapList() {
    if(needsStyling())
      style();
    if(_wrapList.isEmpty())
      computeWrapList();
    return _wrapList;
  }

private:
  inline void style() {
    QtUiStyle::StyledMessage m = QtUi::style()->styleMessage(*_msgBuffer);

    timestamp = new ChatLinePart(m.timestamp.plainText, m.timestamp.formatList);
    sender = new ChatLinePart(m.sender.plainText, m.sender.formatList);
    contents = new ChatLinePart(m.contents.plainText, m.contents.formatList);

    delete _msgBuffer;
    _msgBuffer = 0;
  }

  inline void computeWrapList() {
    if(contents->plainText.isEmpty())
      return;

    enum Mode { SearchStart, SearchEnd };

    QList<ChatLineModel::Word> wplist;  // use a temp list which we'll later copy into a QVector for efficiency
    QTextBoundaryFinder finder(QTextBoundaryFinder::Word, contents->plainText.unicode(), contents->plainText.length(), TextBoundaryFinderBuffer, TextBoundaryFinderBufferSize);

    int idx;
    int oldidx = 0;
    bool wordStart = false;
    bool wordEnd = false;
    Mode mode = SearchEnd;
    ChatLineModel::Word word;
    word.start = 0;
    qreal wordstartx = 0;

    QTextLayout layout(contents->plainText);
    QTextOption option;
    option.setWrapMode(QTextOption::NoWrap);
    layout.setTextOption(option);

    layout.setAdditionalFormats(QtUi::style()->toTextLayoutList(contents->formatList, contents->plainText.length()));
    layout.beginLayout();
    QTextLine line = layout.createLine();
    line.setNumColumns(contents->plainText.length());
    layout.endLayout();

    do {
      idx = finder.toNextBoundary();
      if(idx < 0) {
	idx = contents->plainText.length();
	wordStart = false;
	wordEnd = false;
	mode = SearchStart;
      } else {
	wordStart = finder.boundaryReasons().testFlag(QTextBoundaryFinder::StartWord);
	wordEnd = finder.boundaryReasons().testFlag(QTextBoundaryFinder::EndWord);
      }

      //if(flg) qDebug() << idx << mode << wordStart << wordEnd << contents->plainText.left(idx) << contents->plainText.mid(idx);

      if(mode == SearchEnd || (!wordStart && wordEnd)) {
	if(wordStart || !wordEnd) continue;
	oldidx = idx;
	mode = SearchStart;
	continue;
      }
      qreal wordendx = line.cursorToX(oldidx);
      qreal trailingendx = line.cursorToX(idx);
      word.endX = wordendx;
      word.width = wordendx - wordstartx;
      word.trailing = trailingendx - wordendx;
      wordstartx = trailingendx;
      wplist.append(word);

      if(wordStart) {
	word.start = idx;
	mode = SearchEnd;
      }
      // the part " || (finder.position() == contents->plainText.length())" shouldn't be necessary
      // but in rare and indeterministic cases Qt states that the end of the text is not a boundary o_O
    } while(finder.isAtBoundary() || (finder.position() == contents->plainText.length()));

    // A QVector needs less space than a QList
    _wrapList.resize(wplist.count());
    for(int i = 0; i < wplist.count(); i++) {
      _wrapList[i] = wplist.at(i);
    }
  }

  ChatLineModel::WrapList _wrapList;
  Message *_msgBuffer;
  ChatLinePart *timestamp, *sender, *contents;

  static unsigned char *TextBoundaryFinderBuffer;
  static int TextBoundaryFinderBufferSize;
};

unsigned char *ChatLineModelItemPrivate::TextBoundaryFinderBuffer = (unsigned char *)malloc(512 * sizeof(HB_CharAttributes_Dummy));
int ChatLineModelItemPrivate::TextBoundaryFinderBufferSize = 512 * (sizeof(HB_CharAttributes_Dummy) / sizeof(unsigned char));


// ****************************************
// the actual ChatLineModelItem
// ****************************************
ChatLineModelItem::ChatLineModelItem(const Message &msg)
  : MessageModelItem(msg),
    _data(new ChatLineModelItemPrivate(msg))
{
}

ChatLineModelItem::~ChatLineModelItem() {
  delete _data;
}

QVariant ChatLineModelItem::data(int column, int role) const {
  if(column < ChatLineModel::TimestampColumn || column > ChatLineModel::ContentsColumn)
    return MessageModelItem::data(column, role);
  MessageModel::ColumnType columnType = (MessageModel::ColumnType)column;

  switch(role) {
  case ChatLineModel::DisplayRole:
    return _data->plainText(columnType);
  case ChatLineModel::FormatRole:
    return QVariant::fromValue<UiStyle::FormatList>(_data->formatList(columnType));
  case ChatLineModel::WrapListRole:
    if(columnType != ChatLineModel::ContentsColumn)
      return QVariant();
    return QVariant::fromValue<ChatLineModel::WrapList>(_data->wrapList());
  }
  return MessageModelItem::data(column, role);
}

