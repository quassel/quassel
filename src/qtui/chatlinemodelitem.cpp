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

ChatLineModelItem::ChatLineModelItem(const Message &msg) : MessageModelItem(msg) {
  QtUiStyle::StyledMessage m = QtUi::style()->styleMessage(msg);

  _timestamp.plainText = m.timestamp.plainText;
  _sender.plainText = m.sender.plainText;
  _contents.plainText = m.contents.plainText;

  _timestamp.formatList = m.timestamp.formatList;
  _sender.formatList = m.sender.formatList;
  _contents.formatList = m.contents.formatList;

  computeWrapList();
}


QVariant ChatLineModelItem::data(int column, int role) const {
  const ChatLinePart *part;

  switch(column) {
    case ChatLineModel::TimestampColumn: part = &_timestamp; break;
    case ChatLineModel::SenderColumn:    part = &_sender; break;
    case ChatLineModel::ContentsColumn:      part = &_contents; break;
    default: return MessageModelItem::data(column, role);
  }

  switch(role) {
    case ChatLineModel::DisplayRole:
      return part->plainText;
    case ChatLineModel::FormatRole:
      return QVariant::fromValue<UiStyle::FormatList>(part->formatList);
    case ChatLineModel::WrapListRole:
      if(column != ChatLineModel::ContentsColumn) return QVariant();
      return QVariant::fromValue<ChatLineModel::WrapList>(_wrapList);
  }

  return MessageModelItem::data(column, role);
}

bool ChatLineModelItem::setData(int column, const QVariant &value, int role) {
  return false;
}

void ChatLineModelItem::computeWrapList() {
  enum Mode { SearchStart, SearchEnd };

  QList<ChatLineModel::Word> wplist;  // use a temp list which we'll later copy into a QVector for efficiency
  QTextBoundaryFinder finder(QTextBoundaryFinder::Word, _contents.plainText);
  int idx, oldidx;
  bool wordStart = false; bool wordEnd = false;
  Mode mode = SearchEnd;
  ChatLineModel::Word word;
  word.start = 0;
  int wordstartx = 0;

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
    if(idx < 0) idx = _contents.plainText.length();
    wordStart = finder.boundaryReasons().testFlag(QTextBoundaryFinder::StartWord);
    wordEnd = finder.boundaryReasons().testFlag(QTextBoundaryFinder::EndWord);

    //qDebug() << wordStart << wordEnd << _contents.plainText.left(idx) << _contents.plainText.mid(idx);

    if(mode == SearchEnd || !wordStart && wordEnd) {
      if(wordStart || !wordEnd) continue;
      oldidx = idx;
      mode = SearchStart;
      continue;
    }
    int wordendx = line.cursorToX(oldidx);
    int trailingendx = line.cursorToX(idx);
    word.width = wordendx - wordstartx;
    word.trailing = trailingendx - wordendx;
    wordstartx = trailingendx;
    wplist.append(word);

    if(wordStart) {
      word.start = idx;
      mode = SearchEnd;
    }
  } while(finder.isAtBoundary());

  // A QVector needs less space than a QList
  _wrapList.resize(wplist.count());
  for(int i = 0; i < wplist.count(); i++) {
    _wrapList[i] = wplist.at(i);
  }
}

