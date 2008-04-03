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

#include "chatline-old.h"
#include "client.h"
#include "network.h"
#include "qtui.h"

//! Construct a ChatLineOld object from a message.
/**
 * \param m   The message to be layouted and rendered
 */
ChatLineOld::ChatLineOld(Message m) {
  hght = 0;

  msg = m;
  selectionMode = None;
  isHighlight = false;
  formatMsg(msg);
}

ChatLineOld::~ChatLineOld() {

}

void ChatLineOld::formatMsg(Message msg) {
  isHighlight = msg.flags() & Message::Highlight;
  QTextOption tsOption, senderOption, textOption;
  styledTimeStamp = QtUi::style()->styleString(msg.formattedTimestamp());
  styledSender = QtUi::style()->styleString(msg.formattedSender());
  styledText = QtUi::style()->styleString(msg.formattedText());
  precomputeLine();
}

QList<ChatLineOld::FormatRange> ChatLineOld::calcFormatRanges(const UiStyle::StyledText &fs) {
  QTextLayout::FormatRange additional;
  additional.start = additional.length = 0;
  return calcFormatRanges(fs, additional);
}

// This function is almost obsolete, since with the new style engine, we already get a list of formats...
// We don't know yet if we keep this implementation of ChatLineOld, so I won't bother making this actually nice.
QList<ChatLineOld::FormatRange> ChatLineOld::calcFormatRanges(const UiStyle::StyledText &_fs,
     const QTextLayout::FormatRange &additional) {
  UiStyle::StyledText fs = _fs;
  QList<FormatRange> ranges;

  if(additional.length > 0) {
    for(int i = 0; i < fs.formats.count(); i++) {
      int oldend = fs.formats[i].start + fs.formats[i].length - 1;
      int addend = additional.start + additional.length - 1;
      if(oldend < additional.start) continue;
      fs.formats[i].length = additional.start - fs.formats[i].start;
      QTextLayout::FormatRange addfmtrng = fs.formats[i];
      addfmtrng.format.merge(additional.format);
      addfmtrng.start = additional.start;
      addfmtrng.length = qMin(oldend, addend) - additional.start + 1;
      fs.formats.insert(++i, addfmtrng);
      if(addend == oldend) break;
      if(addend < oldend) {
        QTextLayout::FormatRange restfmtrng = fs.formats[i-1];
        restfmtrng.start = addend + 1;
        restfmtrng.length = oldend - addend;
        fs.formats.insert(++i, restfmtrng);
        break;
      }
    }
  }

  foreach(QTextLayout::FormatRange f, fs.formats) {
    if(f.length <= 0) continue;
    FormatRange range;
    range.start = f.start;
    range.length = f.length;
    range.format = f.format;
    QFontMetrics metrics(range.format.font());
    range.height = metrics.lineSpacing();
    ranges.append(range);
  }
  return ranges;
}

void ChatLineOld::setSelection(SelectionMode mode, int start, int end) {
  selectionMode = mode;
  //tsFormat.clear(); senderFormat.clear(); textFormat.clear();
  QPalette pal = QApplication::palette();
  QTextLayout::FormatRange tsSel, senderSel, textSel;
  switch (mode) {
    case None:
      tsFormat = calcFormatRanges(styledTimeStamp);
      senderFormat = calcFormatRanges(styledSender);
      textFormat = calcFormatRanges(styledText);
      break;
    case Partial:
      selectionStart = qMin(start, end); selectionEnd = qMax(start, end);
      textSel.format.setForeground(pal.brush(QPalette::HighlightedText));
      textSel.format.setBackground(pal.brush(QPalette::Highlight));
      textSel.start = selectionStart;
      textSel.length = selectionEnd - selectionStart;
      textFormat = calcFormatRanges(styledText, textSel);
      break;
    case Full:
      tsSel.format.setForeground(pal.brush(QPalette::HighlightedText));
      tsSel.format.setBackground(pal.brush(QPalette::Highlight));
      tsSel.start = 0; tsSel.length = styledTimeStamp.text.length();
      tsFormat = calcFormatRanges(styledTimeStamp, tsSel);
      senderSel.format.setForeground(pal.brush(QPalette::HighlightedText));
      senderSel.format.setBackground(pal.brush(QPalette::Highlight));
      senderSel.start = 0; senderSel.length = styledSender.text.length();
      senderFormat = calcFormatRanges(styledSender, senderSel);
      textSel.format.setForeground(pal.brush(QPalette::HighlightedText));
      textSel.format.setBackground(pal.brush(QPalette::Highlight));
      textSel.start = 0; textSel.length = styledText.text.length();
      textFormat = calcFormatRanges(styledText, textSel);
      break;
  }
}

MsgId ChatLineOld::msgId() const {
  return msg.msgId();
}

BufferInfo ChatLineOld::bufferInfo() const {
  return msg.bufferInfo();
}

QDateTime ChatLineOld::timestamp() const {
  return msg.timestamp();
}

QString ChatLineOld::sender() const {
  return styledSender.text;
}

QString ChatLineOld::text() const {
  return styledText.text;
}

bool ChatLineOld::isUrl(int c) const {
  if(c < 0 || c >= charUrlIdx.count()) return false;;
  return charUrlIdx[c] >= 0;
}

QUrl ChatLineOld::getUrl(int c) const {
  if(c < 0 || c >= charUrlIdx.count()) return QUrl();
  int i = charUrlIdx[c];
  if(i >= 0) return styledText.urls[i].url;
  else return QUrl();
}

//!\brief Return the cursor position for the given coordinate pos.
/**
 * \param pos The position relative to the ChatLineOld
 * \return The cursor position, [or -3 for invalid,] or -2 for timestamp, or -1 for sender
 */
int ChatLineOld::posToCursor(QPointF pos) {
  if(pos.x() < tsWidth + (int)QtUi::style()->sepTsSender()/2) return -2;
  qreal textStart = tsWidth + QtUi::style()->sepTsSender() + senderWidth + QtUi::style()->sepSenderText();
  if(pos.x() < textStart) return -1;
  int x = (int)(pos.x() - textStart);
  for(int l = lineLayouts.count() - 1; l >=0; l--) {
    LineLayout line = lineLayouts[l];
    if(pos.y() >= line.y) {
      int offset = charPos[line.start]; x += offset;
      for(int i = line.start + line.length - 1; i >= line.start; i--) {
        if((charPos[i] + charPos[i+1])/2 <= x) return i+1; // FIXME: Optimize this!
      }
      return line.start;
    }
  }
  return 0;
}

void ChatLineOld::precomputeLine() {
  tsFormat = calcFormatRanges(styledTimeStamp);
  senderFormat = calcFormatRanges(styledSender);
  textFormat = calcFormatRanges(styledText);

  minHeight = 0;
  foreach(FormatRange fr, tsFormat) minHeight = qMax(minHeight, fr.height);
  foreach(FormatRange fr, senderFormat) minHeight = qMax(minHeight, fr.height);

  words.clear();
  charPos.resize(styledText.text.length() + 1);
  charHeights.resize(styledText.text.length());
  charUrlIdx.fill(-1, styledText.text.length());
  for(int i = 0; i < styledText.urls.count(); i++) {
    QtUiStyle::UrlInfo url = styledText.urls[i];
    for(int j = url.start; j < url.end; j++) charUrlIdx[j] = i;
  }
  if(!textFormat.count()) return;
  int idx = 0; int cnt = 0; int w = 0;
  QFontMetrics metrics(textFormat[0].format.font());
  Word wr;
  wr.start = -1; wr.trailing = -1;
  for(int i = 0; i < styledText.text.length(); ) {
    charPos[i] = w; charHeights[i] = textFormat[idx].height;
    w += metrics.charWidth(styledText.text, i);
    if(!styledText.text[i].isSpace()) {
      if(wr.trailing >= 0) {
        // new word after space
        words.append(wr);
        wr.start = -1;
      }
      if(wr.start < 0) {
        wr.start = i; wr.length = 1; wr.trailing = -1; wr.height = textFormat[idx].height;
      } else {
        wr.length++; wr.height = qMax(wr.height, textFormat[idx].height);
      }
    } else {
      if(wr.start < 0) {
        wr.start = i; wr.length = 0; wr.trailing = 1; wr.height = 0;
      } else {
        wr.trailing++;
      }
    }
    if(++i < styledText.text.length() && ++cnt >= textFormat[idx].length) {
      cnt = 0; idx++;
      Q_ASSERT(idx < textFormat.count());
      metrics = QFontMetrics(textFormat[idx].format.font());
    }
  }
  charPos[styledText.text.length()] = w;
  if(wr.start >= 0) words.append(wr);
}

qreal ChatLineOld::layout(qreal tsw, qreal senderw, qreal textw) {
  tsWidth = tsw; senderWidth = senderw; textWidth = textw;
  if(textw <= 0) return minHeight;
  lineLayouts.clear(); LineLayout line;
  int h = 0;
  int offset = 0; int numWords = 0;
  line.y = 0;
  line.start = 0;
  line.height = minHeight;  // first line needs room for ts and sender
  for(uint i = 0; i < (uint)words.count(); i++) {
    int lastpos = charPos[words[i].start + words[i].length]; // We use charPos[lastchar + 1], 'coz last char needs to fit
    if(lastpos - offset <= textw) {
      line.height = qMax(line.height, words[i].height);
      line.length = words[i].start + words[i].length - line.start;
      numWords++;
    } else {
      // we need to wrap!
      if(numWords > 0) {
        // ok, we had some words before, so store the layout and start a new line
        h += line.height;
        line.length = words[i-1].start + words[i-1].length - line.start;
        lineLayouts.append(line);
        line.y += line.height;
        line.start = words[i].start;
        line.height = words[i].height;
        offset = charPos[words[i].start];
      }
      numWords = 1;
      // check if the word fits into the current line
      if(lastpos - offset <= textw) {
        line.length = words[i].length;
      } else {
        // we need to break a word in the middle
        int border = (int)textw + offset; // save some additions
        line.start = words[i].start;
        line.length = 1;
        line.height = charHeights[line.start];
        int j = line.start + 1;
        for(int l = 1; l < words[i].length; j++, l++) {
          if(charPos[j+1] < border) {
            line.length++;
            line.height = qMax(line.height, charHeights[j]);
            continue;
          } else {
            h += line.height;
            lineLayouts.append(line);
            line.y += line.height;
            line.start = j;
            line.height = charHeights[j];
            line.length = 1;
            offset = charPos[j];
            border = (int)textw + offset;
          }
        }
      }
    }
  }
  h += line.height;
  if(numWords > 0) {
    lineLayouts.append(line);
  }
  hght = h;
  return hght;
}

//!\brief Draw ChatLineOld on the given QPainter at the given position.
void ChatLineOld::draw(QPainter *p, const QPointF &pos) {
  QPalette pal = QApplication::palette();

  if(selectionMode == Full) {
    p->setPen(Qt::NoPen);
    p->setBrush(pal.brush(QPalette::Highlight));
    p->drawRect(QRectF(pos, QSizeF(tsWidth + QtUi::style()->sepTsSender() + senderWidth + QtUi::style()->sepSenderText() + textWidth, height())));
  } else {
    if(isHighlight) {
      p->setPen(Qt::NoPen);
      p->setBrush(QColor("lightcoral") /*pal.brush(QPalette::AlternateBase) */);
      p->drawRect(QRectF(pos, QSizeF(tsWidth + QtUi::style()->sepTsSender() + senderWidth + QtUi::style()->sepSenderText() + textWidth, height())));
    }
    if(selectionMode == Partial) {

    }
  } /*
  p->setClipRect(QRectF(pos, QSizeF(tsWidth, height())));
  tsLayout.draw(p, pos, tsFormat);
  p->setClipRect(QRectF(pos + QPointF(tsWidth + Style::sepTsSender(), 0), QSizeF(senderWidth, height())));
  senderLayout.draw(p, pos + QPointF(tsWidth + Style::sepTsSender(), 0), senderFormat);
  p->setClipping(false);
  textLayout.draw(p, pos + QPointF(tsWidth + Style::sepTsSender() + senderWidth + Style::sepSenderText(), 0), textFormat);
    */
  //p->setClipRect(QRectF(pos, QSizeF(tsWidth, 15)));
  //p->drawRect(QRectF(pos, QSizeF(tsWidth, minHeight)));
  p->setBackgroundMode(Qt::OpaqueMode);
  QPointF tp = pos;
  QRectF rect(pos, QSizeF(tsWidth, minHeight));
  QRectF brect;
  foreach(FormatRange fr, tsFormat) {
    p->setFont(fr.format.font());
    p->setPen(QPen(fr.format.foreground(), 0)); p->setBackground(fr.format.background());
    p->drawText(rect, Qt::AlignLeft|Qt::TextSingleLine, styledTimeStamp.text.mid(fr.start, fr.length), &brect);
    rect.setLeft(brect.right());
  }
  rect = QRectF(pos + QPointF(tsWidth + QtUi::style()->sepTsSender(), 0), QSizeF(senderWidth, minHeight));
  for(int i = senderFormat.count() - 1; i >= 0; i--) {
    FormatRange fr = senderFormat[i];
    p->setFont(fr.format.font()); p->setPen(QPen(fr.format.foreground(), 0)); p->setBackground(fr.format.background());
    p->drawText(rect, Qt::AlignRight|Qt::TextSingleLine, styledSender.text.mid(fr.start, fr.length), &brect);
    rect.setRight(brect.left());
  }
  QPointF tpos = pos + QPointF(tsWidth + QtUi::style()->sepTsSender() + senderWidth + QtUi::style()->sepSenderText(), 0);
  qreal h = 0; int l = 0;
  if(lineLayouts.count() == 0) return; // how can this happen?
  rect = QRectF(tpos + QPointF(0, h), QSizeF(textWidth, lineLayouts[l].height));
  int offset = 0;
  foreach(FormatRange fr, textFormat) {
    if(l >= lineLayouts.count()) break;
    p->setFont(fr.format.font()); p->setPen(QPen(fr.format.foreground(), 0)); p->setBackground(fr.format.background());
    int start, end, frend, llend;
    do {
      frend = fr.start + fr.length;
      if(frend <= lineLayouts[l].start) break;
      llend = lineLayouts[l].start + lineLayouts[l].length;
      start = qMax(fr.start, lineLayouts[l].start); end = qMin(frend, llend);
      rect.setLeft(tpos.x() + charPos[start] - offset);
      p->drawText(rect, Qt::AlignLeft|Qt::TextSingleLine, styledText.text.mid(start, end - start), &brect);
      if(llend <= end) {
        h += lineLayouts[l].height;
        l++;
        if(l < lineLayouts.count()) {
          rect = QRectF(tpos + QPointF(0, h), QSizeF(textWidth, lineLayouts[l].height));
          offset = charPos[lineLayouts[l].start];
        }
      }
    } while(end < frend && l < lineLayouts.count());
  }
}
