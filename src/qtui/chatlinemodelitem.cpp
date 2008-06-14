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
      QVariantList wrapList;
      typedef QPair<quint16, quint16> WrapPoint;  // foreach can't parse templated params
      foreach(WrapPoint pair, _wrapList) wrapList << pair.first << pair.second;
      return wrapList;
  }

  return MessageModelItem::data(column, role);
}

bool ChatLineModelItem::setData(int column, const QVariant &value, int role) {
  return false;
}

void ChatLineModelItem::computeWrapList() {
  WrapList wplist;  // use a temp list which we'll later copy into a QVector for efficiency
  QTextBoundaryFinder finder(QTextBoundaryFinder::Word, _contents.plainText);
  int idx;
  int flistidx = -1;
  int fmtend = -1;
  QFontMetricsF *metrics;
  QPair<quint16, quint16> wp(0, 0);
  do {
    idx = finder.toNextBoundary();
    if(idx < 0) idx = _contents.plainText.length();
    else if(finder.boundaryReasons() != QTextBoundaryFinder::StartWord) continue;
    int start = wp.first;
    while(start < idx) {
      if(fmtend <= start) {
        flistidx++;
        fmtend = _contents.formatList.count() > flistidx+1 ? _contents.formatList[flistidx+1].first
                                                           : _contents.plainText.length();
        metrics = QtUi::style()->fontMetrics(_contents.formatList[flistidx].second);
        Q_ASSERT(fmtend > start);
      }
      int i = qMin(idx, fmtend);
      wp.second += metrics->width(_contents.plainText.mid(start, i - start));
      start = i;
    }
    wplist.append(wp);
    wp.first = idx;
  } while(finder.isAtBoundary());

  // A QVector needs less space than a QList
  _wrapList.resize(wplist.count());
  for(int i = 0; i < wplist.count(); i++) {
    _wrapList[i] = wplist.at(i);
  }
}

