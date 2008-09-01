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
#include "uistyle.h"

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

unsigned char *ChatLineModelItem::TextBoundaryFinderBuffer = (unsigned char *)malloc(512 * sizeof(HB_CharAttributes_Dummy));
int ChatLineModelItem::TextBoundaryFinderBufferSize = 512 * (sizeof(HB_CharAttributes_Dummy) / sizeof(unsigned char));

struct ChatLineModelItemPrivate {
  ChatLineModel::WrapList wrapList;
};

ChatLineModelItem::ChatLineModelItem(const Message &msg)
  : MessageModelItem(msg),
    _data(new ChatLineModelItemPrivate)
{
  QtUiStyle::StyledMessage m = QtUi::style()->styleMessage(msg);

  _timestamp.plainText = m.timestamp.plainText;
  _sender.plainText = m.sender.plainText;
  _contents.plainText = m.contents.plainText;

  _timestamp.formatList = m.timestamp.formatList;
  _sender.formatList = m.sender.formatList;
  _contents.formatList = m.contents.formatList;
}

ChatLineModelItem::~ChatLineModelItem() {
  delete _data;
}

QVariant ChatLineModelItem::data(int column, int role) const {
  const ChatLinePart *part = 0;

  switch(column) {
  case ChatLineModel::TimestampColumn:
    part = &_timestamp;
    break;
  case ChatLineModel::SenderColumn:
    part = &_sender;
    break;
  case ChatLineModel::ContentsColumn:
    part = &_contents;
    break;
  default:
    return MessageModelItem::data(column, role);
  }

  switch(role) {
  case ChatLineModel::DisplayRole:
    return part->plainText;
  case ChatLineModel::FormatRole:
    return QVariant::fromValue<UiStyle::FormatList>(part->formatList);
  case ChatLineModel::WrapListRole:
    if(column != ChatLineModel::ContentsColumn)
      return QVariant();
    if(_data->wrapList.isEmpty())
      computeWrapList();
    return QVariant::fromValue<ChatLineModel::WrapList>(_data->wrapList);
  }
  return MessageModelItem::data(column, role);
}

void ChatLineModelItem::computeWrapList() const {
  if(_contents.plainText.isEmpty())
    return;

  enum Mode { SearchStart, SearchEnd };

  QList<ChatLineModel::Word> wplist;  // use a temp list which we'll later copy into a QVector for efficiency
  QTextBoundaryFinder finder(QTextBoundaryFinder::Word, _contents.plainText.unicode(), _contents.plainText.length(), TextBoundaryFinderBuffer, TextBoundaryFinderBufferSize);

  int idx;
  int oldidx = 0;
  bool wordStart = false;
  bool wordEnd = false;
  Mode mode = SearchEnd;
  ChatLineModel::Word word;
  word.start = 0;
  qreal wordstartx = 0;

  QTextLayout layout(_contents.plainText);
  QTextOption option;
  option.setWrapMode(QTextOption::NoWrap);
  layout.setTextOption(option);

  layout.setAdditionalFormats(QtUi::style()->toTextLayoutList(_contents.formatList, _contents.plainText.length()));
  layout.beginLayout();
  QTextLine line = layout.createLine();
  line.setNumColumns(_contents.plainText.length());
  layout.endLayout();

  do {
    idx = finder.toNextBoundary();
    if(idx < 0) {
      idx = _contents.plainText.length();
      wordStart = false;
      wordEnd = false;
      mode = SearchStart;
    } else {
      wordStart = finder.boundaryReasons().testFlag(QTextBoundaryFinder::StartWord);
      wordEnd = finder.boundaryReasons().testFlag(QTextBoundaryFinder::EndWord);
    }

    //if(flg) qDebug() << idx << mode << wordStart << wordEnd << _contents.plainText.left(idx) << _contents.plainText.mid(idx);

    if(mode == SearchEnd || (!wordStart && wordEnd)) {
      if(wordStart || !wordEnd) continue;
      oldidx = idx;
      mode = SearchStart;
      continue;
    }
    qreal wordendx = line.cursorToX(oldidx);
    qreal trailingendx = line.cursorToX(idx);
    word.width = wordendx - wordstartx;
    word.trailing = trailingendx - wordendx;
    wordstartx = trailingendx;
    wplist.append(word);

    if(wordStart) {
      word.start = idx;
      mode = SearchEnd;
    }
    // the part " || (finder.position() == _contents.plainText.length())" shouldn't be necessary
    // but in rare and indeterministic cases Qt states that the end of the text is not a boundary o_O
  } while(finder.isAtBoundary() || (finder.position() == _contents.plainText.length()));

  // A QVector needs less space than a QList
  _data->wrapList.resize(wplist.count());
  for(int i = 0; i < wplist.count(); i++) {
    _data->wrapList[i] = wplist.at(i);
  }
}

