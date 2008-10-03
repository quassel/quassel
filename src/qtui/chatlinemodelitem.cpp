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

public:
  inline ChatLineModelItemPrivate(const Message &msg);
  ~ChatLineModelItemPrivate();

  inline bool needsStyling();
  QVariant data(MessageModel::ColumnType column, int role);

private:
  void style();
  void computeWrapList();

  ChatLineModel::WrapList _wrapList;
  Message *_msgBuffer;
  UiStyle::StyledMessage *_styledMsg;

  static unsigned char *TextBoundaryFinderBuffer;
  static int TextBoundaryFinderBufferSize;
};

ChatLineModelItemPrivate::ChatLineModelItemPrivate(const Message &msg)
: _msgBuffer(new Message(msg)),
  _styledMsg(0) {

}

ChatLineModelItemPrivate::~ChatLineModelItemPrivate() {
  if(_msgBuffer) {
    delete _msgBuffer;
  } else {
    delete _styledMsg;
  }
}

bool ChatLineModelItemPrivate::needsStyling() {
  return (bool)_msgBuffer;
}

QVariant ChatLineModelItemPrivate::data(MessageModel::ColumnType column, int role) {
  if(needsStyling())
    style();
  switch(column) {
    case ChatLineModel::TimestampColumn:
      switch(role) {
        case ChatLineModel::DisplayRole:
          return _styledMsg->decoratedTimestamp();
        case ChatLineModel::EditRole:
          return _styledMsg->timestamp();
        case ChatLineModel::FormatRole:
          return QVariant::fromValue<UiStyle::FormatList>(UiStyle::FormatList()
                                    << qMakePair((quint16)0, (quint32)_styledMsg->timestampFormat()));
      }
      break;
    case ChatLineModel::SenderColumn:
      switch(role) {
        case ChatLineModel::DisplayRole:
          return _styledMsg->decoratedSender();
        case ChatLineModel::EditRole:
          return _styledMsg->sender();
        case ChatLineModel::FormatRole:
          return QVariant::fromValue<UiStyle::FormatList>(UiStyle::FormatList()
                                    << qMakePair((quint16)0, (quint32)_styledMsg->senderFormat()));
      }
      break;
    case ChatLineModel::ContentsColumn:
      switch(role) {
        case ChatLineModel::DisplayRole:
        case ChatLineModel::EditRole:
          return _styledMsg->contents();
        case ChatLineModel::FormatRole:
          return QVariant::fromValue<UiStyle::FormatList>(_styledMsg->contentsFormatList());
        case ChatLineModel::WrapListRole:
          if(_wrapList.isEmpty())
            computeWrapList();
          return QVariant::fromValue<ChatLineModel::WrapList>(_wrapList);
      }
      break;
    default:
      Q_ASSERT(false);
      return 0;
  }
  return QVariant();
}

void ChatLineModelItemPrivate::style() {
  _styledMsg = new QtUiStyle::StyledMessage(QtUi::style()->styleMessage(*_msgBuffer));
  delete _msgBuffer;
  _msgBuffer = 0;
}

void ChatLineModelItemPrivate::computeWrapList() {
  int length = _styledMsg->contents().length();
  if(!length)
    return;

  enum Mode { SearchStart, SearchEnd };

  QList<ChatLineModel::Word> wplist;  // use a temp list which we'll later copy into a QVector for efficiency
  QTextBoundaryFinder finder(QTextBoundaryFinder::Word, _styledMsg->contents().unicode(), length,
                              TextBoundaryFinderBuffer, TextBoundaryFinderBufferSize);

  int idx;
  int oldidx = 0;
  bool wordStart = false;
  bool wordEnd = false;
  Mode mode = SearchEnd;
  ChatLineModel::Word word;
  word.start = 0;
  qreal wordstartx = 0;

  QTextLayout layout(_styledMsg->contents());
  QTextOption option;
  option.setWrapMode(QTextOption::NoWrap);
  layout.setTextOption(option);

  layout.setAdditionalFormats(QtUi::style()->toTextLayoutList(_styledMsg->contentsFormatList(), length));
  layout.beginLayout();
  QTextLine line = layout.createLine();
  line.setNumColumns(length);
  layout.endLayout();

  do {
    idx = finder.toNextBoundary();
    if(idx < 0) {
      idx = length;
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
  } while(finder.isAtBoundary() || (finder.position() == length));

  // A QVector needs less space than a QList
  _wrapList.resize(wplist.count());
  for(int i = 0; i < wplist.count(); i++) {
    _wrapList[i] = wplist.at(i);
  }
}

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
  QVariant d = _data->data((MessageModel::ColumnType)column, role);
  if(!d.isValid()) return MessageModelItem::data(column, role);
  return d;
}
