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

#include "uistyle.h"
#include "uistylesettings.h"

UiStyle::UiStyle(const QString &settingsKey) : _settingsKey(settingsKey) {
  // Default format
  QTextCharFormat def;
  def.setForeground(QBrush("#000000"));
  //def.setFont(QFont("Courier", 10));
  def.font().setStyleHint(QFont::TypeWriter);
  _defaultFormats = QVector<QTextCharFormat>(NumFormatTypes, def);
  _customFormats = QVector<QTextCharFormat>(NumFormatTypes, QTextFormat().toCharFormat());

  // Load saved custom formats
  UiStyleSettings s(_settingsKey);
  foreach(FormatType type, s.availableFormats()) {
    _customFormats[type] = s.customFormat(type);
  }

  // Initialize color codes according to mIRC "standard"
  QStringList colors;
  //colors << "white" << "black" << "navy" << "green" << "red" << "maroon" << "purple" << "orange";
  //colors << "yellow" << "lime" << "teal" << "aqua" << "royalblue" << "fuchsia" << "grey" << "silver";
  colors << "#ffffff" << "#000000" << "#000080" << "#008000" << "#ff0000" << "#800000" << "#800080" << "#ffa500";
  colors << "#ffff00" << "#00ff00" << "#008080" << "#00ffff" << "#4169E1" << "#ff00ff" << "#808080" << "#c0c0c0";

  // Now initialize the mapping between FormatCodes and FormatTypes...
  _formatCodes["%O"] = None;
  _formatCodes["%B"] = Bold;
  _formatCodes["%S"] = Italic;
  _formatCodes["%U"] = Underline;
  _formatCodes["%R"] = Reverse;

  _formatCodes["%D0"] = PlainMsg;
  _formatCodes["%Dn"] = NoticeMsg;
  _formatCodes["%Ds"] = ServerMsg;
  _formatCodes["%De"] = ErrorMsg;
  _formatCodes["%Dj"] = JoinMsg;
  _formatCodes["%Dp"] = PartMsg;
  _formatCodes["%Dq"] = QuitMsg;
  _formatCodes["%Dk"] = KickMsg;
  _formatCodes["%Dr"] = RenameMsg;
  _formatCodes["%Dm"] = ModeMsg;
  _formatCodes["%Da"] = ActionMsg;

  _formatCodes["%DT"] = Timestamp;
  _formatCodes["%DS"] = Sender;
  _formatCodes["%DN"] = Nick;
  _formatCodes["%DH"] = Hostmask;
  _formatCodes["%DC"] = ChannelName;
  _formatCodes["%DM"] = ModeFlags;
  _formatCodes["%DU"] = Url;

  // Set color formats
  for(int i = 0; i < 16; i++) {
    QString idx = QString("%1").arg(i, (int)2, (int)10, (QChar)'0');
    _formatCodes[QString("%Dcf%1").arg(idx)] = (FormatType)(FgCol00 + i);
    _formatCodes[QString("%Dcb%1").arg(idx)] = (FormatType)(BgCol00 + i);
    QTextCharFormat fgf, bgf;
    fgf.setForeground(QBrush(QColor(colors[i]))); setFormat((FormatType)(FgCol00 + i), fgf, Settings::Default);
    bgf.setBackground(QBrush(QColor(colors[i]))); setFormat((FormatType)(BgCol00 + i), bgf, Settings::Default);
    //FIXME fix the havoc caused by ColorSettingsPage
    setFormat((FormatType)(FgCol00 + i), fgf, Settings::Custom);
    setFormat((FormatType)(BgCol00 + i), bgf, Settings::Custom);
  }

  // Set a few more standard formats
  QTextCharFormat bold; bold.setFontWeight(QFont::Bold);
  setFormat(Bold, bold, Settings::Default);

  QTextCharFormat italic; italic.setFontItalic(true);
  setFormat(Italic, italic, Settings::Default);

  QTextCharFormat underline; underline.setFontUnderline(true);
  setFormat(Underline, underline, Settings::Default);

  // All other formats should be defined in derived classes.
}

UiStyle::~ UiStyle() {
  
}

void UiStyle::setFormat(FormatType ftype, QTextCharFormat fmt, Settings::Mode mode) {
  if(mode == Settings::Default) {
    _defaultFormats[ftype] = fmt;
  } else {
    UiStyleSettings s(_settingsKey);
    if(fmt != _defaultFormats[ftype]) {
      _customFormats[ftype] = fmt;
      s.setCustomFormat(ftype, fmt);
    } else {
      _customFormats[ftype] = QTextFormat().toCharFormat();
      s.removeCustomFormat(ftype);
    }
  }
}

QTextCharFormat UiStyle::format(FormatType ftype, Settings::Mode mode) const {
  if(mode == Settings::Custom && _customFormats[ftype].isValid()) return _customFormats[ftype];
  else return _defaultFormats[ftype];
}

UiStyle::FormatType UiStyle::formatType(const QString & code) const {
  if(_formatCodes.contains(code)) return _formatCodes.value(code);
  return Invalid;
}

QString UiStyle::formatCode(FormatType ftype) const {
  return _formatCodes.key(ftype);
}

UiStyle::StyledText UiStyle::styleString(QString s) {
  StyledText result;
  QList<FormatType> fmtList;
  fmtList.append(None);
  QTextLayout::FormatRange curFmtRng;
  curFmtRng.format = format(None);
  curFmtRng.start = 0;
  result.formats.append(curFmtRng);
  int pos = 0; int length;
  int fgCol = -1, bgCol = -1;  // marks current mIRC color
  for(;;) {
    pos = s.indexOf('%', pos);
    if(pos < 0) break;
    if(s[pos+1] == '%') { // escaped %, just remove one and continue
      s.remove(pos, 1);
      pos++;
      continue;
    } else if(s[pos+1] == 'D' && s[pos+2] == 'c') { // color code
      if(s[pos+3] == '-') { // color off
        if(fgCol >= 0) {
          fmtList.removeAll((FormatType)(FgCol00 + fgCol));
          fgCol = -1;
        }
        if(bgCol >= 0) {
          fmtList.removeAll((FormatType)(BgCol00 + bgCol));
          bgCol = -1;
        }
        curFmtRng.format = mergedFormat(fmtList);
        length = 4;
      } else {
        int color = 10 * s[pos+4].digitValue() + s[pos+5].digitValue();
        //TODO: use 99 as transparent color (re mirc color "standard")
        color &= 0x0f;
        int *colptr; FormatType coltype;
        if(s[pos+3] == 'f') { // foreground
          colptr = &fgCol; coltype = FgCol00;
        } else {              // background
          Q_ASSERT(s[pos+3] == 'b');
          colptr = &bgCol; coltype = BgCol00;
        }
        if(*colptr >= 0) {
          // color already set, remove format code and add new one
          Q_ASSERT(fmtList.contains((FormatType)(coltype + *colptr)));
          fmtList.removeAll((FormatType)(coltype + *colptr));
          fmtList.append((FormatType)(coltype + color));
          curFmtRng.format = mergedFormat(fmtList);
        } else {
          fmtList.append((FormatType)(coltype + color));
          curFmtRng.format.merge(format(fmtList.last()));
        }
        *colptr = color;
        length = 6;
      }
    } else if(s[pos+1] == 'O') { // reset formatting
      fmtList.clear(); fmtList.append(None);
      curFmtRng.format = format(None);
      fgCol = bgCol = -1;
      length = 2;
    } else if(s[pos+1] == 'R') { // reverse
      // TODO: implement reverse formatting

      length = 2;
    } else { // all others are toggles
      QString code = QString("%") + s[pos+1];
      if(s[pos+1] == 'D') code += s[pos+2];
      FormatType ftype = formatType(code);
      if(ftype == Invalid) {
        qWarning(qPrintable(QString("Invalid format code in string: %1").arg(s)));
        continue;
      }
      //Q_ASSERT(ftype != Invalid);
      length = code.length();
      if(!fmtList.contains(ftype)) {
        // toggle it on
        fmtList.append(ftype);
        curFmtRng.format.merge(format(ftype));
      } else {
        // toggle it off
        fmtList.removeAll(ftype);
        curFmtRng.format = mergedFormat(fmtList);
      }
    }
    s.remove(pos, length); // remove format code from string
    // now see if something changed and else insert the format
    if(curFmtRng.format == result.formats.last().format) continue;  // no change, so we just ignore
    curFmtRng.start = pos;
    if(pos == result.formats.last().start) {
      // same starting point -> we just overwrite the old format
      result.formats.last() = curFmtRng;
    } else {
      // fix length of last format
      result.formats.last().length = pos - result.formats.last().start;
      result.formats.append(curFmtRng);
    }
  }
  result.formats.last().length = s.length() - result.formats.last().start;
  if(result.formats.last().length == 0) result.formats.removeLast();
  result.text = s;
  return result;
}

QTextCharFormat UiStyle::mergedFormat(QList<FormatType> formatList) {
  QTextCharFormat fmt;
  foreach(FormatType ftype, formatList) {
    fmt.merge(format(ftype));
  }
  return fmt;
}


