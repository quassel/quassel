/***************************************************************************
 *   Copyright (C) 2005-07 by The Quassel Team                             *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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

#include "style.h"

void Style::init() {
   // Colors (mIRC standard)
  colors["00"] = QColor("white");
  colors["01"] = QColor("black");
  colors["02"] = QColor("navy");
  colors["03"] = QColor("green");
  colors["04"] = QColor("red");
  colors["05"] = QColor("maroon");
  colors["06"] = QColor("purple");
  colors["07"] = QColor("orange");
  colors["08"] = QColor("yellow");
  colors["09"] = QColor("lime");
  colors["10"] = QColor("teal");
  colors["11"] = QColor("aqua");
  colors["12"] = QColor("royalblue");
  colors["13"] = QColor("fuchsia");
  colors["14"] = QColor("grey");
  colors["15"] = QColor("silver");

  QTextCharFormat def;
  def.setForeground(QBrush("black"));
  def.setFont(QFont("Verdana",9));
  formats["default"] = def;

  // %B - 0x02 - bold
  QTextCharFormat bold;
  bold.setFontWeight(QFont::Bold);
  formats["%B"] = bold;

  // %O - 0x0f - plain
  formats["%O"] = def;

  // %R - 0x12 - reverse
  // -- - 0x16 - reverse
  // (no format)

  // %S - 0x1d - italic
  QTextCharFormat italic;
  italic.setFontItalic(true);
  formats["%S"] = italic;

  // %U - 0x1f - underline
  QTextCharFormat underline;
  underline.setFontUnderline(true);
  formats["%U"] = underline;

  // %C - 0x03 - mIRC colors
  for(uint i = 0; i < 16; i++) {
    QString idx = QString("%1").arg(i, (int)2, (int)10, (QChar)'0');
    QString fg = QString("%C%1").arg(idx);
    QString bg = QString("%C,%1").arg(idx);
    QTextCharFormat fgf; fgf.setForeground(QBrush(colors[idx])); formats[fg] = fgf;
    QTextCharFormat bgf; bgf.setBackground(QBrush(colors[idx])); formats[bg] = bgf;
  }

  // Internal formats - %D<char>
  // %D0 - plain msg
  QTextCharFormat plainMsg;
  plainMsg.setForeground(QBrush("black"));
  formats["%D0"] = plainMsg;
  // %Dn - notice
  QTextCharFormat notice;
  notice.setForeground(QBrush("navy"));
  formats["%Dn"] = notice;
  // %Ds - server msg
  QTextCharFormat server;
  server.setForeground(QBrush("navy"));
  formats["%Ds"] = server;
  // %De - error msg
  QTextCharFormat error;
  error.setForeground(QBrush("red"));
  formats["%De"] = error;
  // %Dj - join
  QTextCharFormat join;
  join.setForeground(QBrush("green"));
  formats["%Dj"] = join;
  // %Dp - part
  QTextCharFormat part;
  part.setForeground(QBrush("indianred"));
  formats["%Dp"] = part;
  // %Dq - quit
  QTextCharFormat quit;
  quit.setForeground(QBrush("indianred"));
  formats["%Dq"] = quit;
  // %Dk - kick
  QTextCharFormat kick;
  kick.setForeground(QBrush("indianred"));
  formats["%Dk"] = kick;
  // %Dr - nick rename
  QTextCharFormat nren;
  nren.setForeground(QBrush("magenta"));
  formats["%Dr"] = nren;
  // %Dm - mode change
  QTextCharFormat mode;
  mode.setForeground(QBrush("steelblue"));
  formats["%Dm"] = mode;
  // %Da - ctcp action
  QTextCharFormat action;
  action.setFontItalic(true);
  action.setForeground(QBrush("darkmagenta"));
  formats["%Da"] = action;

  // %DT - timestamp
  QTextCharFormat ts;
  ts.setForeground(QBrush("grey"));
  formats["%DT"] = ts;
  // %DS - sender
  QTextCharFormat sender;
  sender.setAnchor(true);
  sender.setForeground(QBrush("navy"));
  formats["%DS"] = sender;
  // %DN - nickname
  QTextCharFormat nick;
  nick.setAnchor(true);
  nick.setFontWeight(QFont::Bold);
  formats["%DN"] = nick;
  // %DH - hostmask
  QTextCharFormat hostmask;
  hostmask.setFontItalic(true);
  formats["%DH"] = hostmask;
  // %DC - channame
  QTextCharFormat channel;
  channel.setAnchor(true);
  channel.setFontWeight(QFont::Bold);
  formats["%DC"] = channel;
  // %DM - modeflags
  QTextCharFormat flags;
  flags.setFontWeight(QFont::Bold);
  formats["%DM"] = flags;
  // %DU - clickable URL
  QTextCharFormat url;
  url.setFontUnderline(true);
  url.setAnchor(true);
  formats["%DU"] = url;
}

QString Style::mircToInternal(QString mirc) {
  mirc.replace('%', "%%");      // escape % just to be sure
  mirc.replace('\x02', "%B");
  mirc.replace('\x03', "%C");
  mirc.replace('\x0f', "%O");
  mirc.replace('\x12', "%R");
  mirc.replace('\x16', "%R");
  mirc.replace('\x1d', "%S");
  mirc.replace('\x1f', "%U");
  return mirc;
}

/** Returns a string stripped of format codes, and a list of FormatRange objects
 *  describing the formats of the string.
 * \param s string in internal format (% style format codes)
 */ 
Style::FormattedString Style::internalToFormatted(QString s) {
  QHash<QString, int> toggles;
  QString p;
  FormattedString sf;
  QTextLayout::FormatRange rng;
  rng.format = formats["default"]; rng.start = 0; rng.length = -1; sf.formats.append(rng);
  toggles["default"] = sf.formats.count() - 1;
  int i, j;
  for(i = 0, j = 0; i < s.length(); i++) {
    if(s[i] != '%') { p += s[i]; j++; continue; }
    i++;
    if(s[i] == '%') { p += '%'; j++; continue; }
    else if(s[i] == 'C') {
      if(!s[i+1].isDigit() && s[i+1] != ',') {
        if(toggles.contains("bg")) {
          sf.formats[toggles["bg"]].length = j - sf.formats[toggles["bg"]].start;
          toggles.remove("bg");
        }
      }
      if(s[i+1].isDigit() || s[i+1] != ',') {
        if(toggles.contains("fg")) {
          sf.formats[toggles["fg"]].length = j - sf.formats[toggles["fg"]].start;
          toggles.remove("fg");
        }
        if(s[i+1].isDigit()) {
          QString n(s[++i]);
          if(s[i+1].isDigit()) n += s[++i];
          int num = n.toInt() & 0xf;
          n = QString("%C%1").arg(num, (int)2, (int)10, (QChar)'0');
          //qDebug() << n << formats[n].foreground();
          QTextLayout::FormatRange range; 
          range.format = formats[n]; range.start = j; range.length = -1; sf.formats.append(range);
          toggles["fg"] = sf.formats.count() - 1;
        }
      }
      if(s[i+1] == ',') {
        if(toggles.contains("bg")) {
          sf.formats[toggles["bg"]].length = j - sf.formats[toggles["bg"]].start;
          toggles.remove("bg");
        }
        i++;
        if(s[i+1].isDigit()) {
          QString n(s[++i]);
          if(s[i+1].isDigit()) n += s[++i];
          int num = n.toInt() & 0xf;
          n = QString("%C,%1").arg(num, (int)2, (int)10, (QChar)'0');
          QTextLayout::FormatRange range;
          range.format = formats[n]; range.start = j; range.length = -1;
          sf.formats.append(range);
          toggles["bg"] = sf.formats.count() - 1;
        }
      }
    } else if(s[i] == 'O') {
      foreach(QString key, toggles.keys()) {
        if(key == "default") continue;
        sf.formats[toggles[key]].length = j - sf.formats[toggles[key]].start;
        toggles.remove(key);
      }

    } else if(s[i] == 'R') {
      // TODO implement reverse formatting

    } else {
      // all others are toggles
      QString key = "%"; key += s[i];
      if(s[i] == 'D') key += s[i+1];
      if(formats.contains(key)) {
        if(s[i] == 'D') i++;
        if(toggles.contains(key)) {
          sf.formats[toggles[key]].length = j - sf.formats[toggles[key]].start;
          if(key == "%DU") {
            // URL handling
            // FIXME check for and handle format codes within URLs
            QString u = s.mid(i - sf.formats[toggles[key]].length - 2, sf.formats[toggles[key]].length);
            UrlInfo url;
            url.start = sf.formats[toggles[key]].start;
            url.end   = j;
            url.url = QUrl(u);
            sf.urls.append(url);
          }
          toggles.remove(key);
        } else {
          QTextLayout::FormatRange range;
          range.format = formats[key]; range.start = j; range.length = -1;
          sf.formats.append(range);
          toggles[key] = sf.formats.count() -1;
        }
      } else {
        // unknown format
        p += '%'; p += s[i]; j+=2;
      }
    }
  }
  foreach(int idx, toggles.values()) {
    sf.formats[idx].length = j - sf.formats[idx].start;
  }
  sf.text = p;
  return sf;
}

QHash<QString, QTextCharFormat> Style::formats;
QHash<QString, QColor> Style::colors;

