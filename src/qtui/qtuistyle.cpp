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

#include "qtuistyle.h"
#include "qtuisettings.h"

QtUiStyle::QtUiStyle() : UiStyle("QtUiStyle") {
  // We need to just set our internal formats; everything else is done by the base class...

  // Internal message formats
  QTextCharFormat plainMsg;
  plainMsg.setForeground(QBrush("black"));
  setFormat(PlainMsg, plainMsg, Settings::Default);

  QTextCharFormat notice;
  notice.setForeground(QBrush("navy"));
  setFormat(NoticeMsg, notice, Settings::Default);

  QTextCharFormat server;
  server.setForeground(QBrush("navy"));
  setFormat(ServerMsg, server, Settings::Default);

  QTextCharFormat error;
  error.setForeground(QBrush("red"));
  setFormat(ErrorMsg, error, Settings::Default);

  QTextCharFormat join;
  join.setForeground(QBrush("green"));
  setFormat(JoinMsg, join, Settings::Default);

  QTextCharFormat part;
  part.setForeground(QBrush("indianred"));
  setFormat(PartMsg, part, Settings::Default);

  QTextCharFormat quit;
  quit.setForeground(QBrush("indianred"));
  setFormat(QuitMsg, quit, Settings::Default);

  QTextCharFormat kick;
  kick.setForeground(QBrush("indianred"));
  setFormat(KickMsg, kick, Settings::Default);

  QTextCharFormat nren;
  nren.setForeground(QBrush("magenta"));
  setFormat(RenameMsg, nren, Settings::Default);

  QTextCharFormat mode;
  mode.setForeground(QBrush("steelblue"));
  setFormat(ModeMsg, mode, Settings::Default);

  QTextCharFormat action;
  action.setFontItalic(true);
  action.setForeground(QBrush("darkmagenta"));
  setFormat(ActionMsg, action, Settings::Default);

  // Internal message element formats
  QTextCharFormat ts;
  ts.setForeground(QBrush("grey"));
  setFormat(Timestamp, ts, Settings::Default);

  // Set the default sender color
  QTextCharFormat sender;
  sender.setAnchor(true);
  sender.setForeground(QBrush("navy"));
  setFormat(Sender, sender, Settings::Default);

  /*
   * Fillup the list of colors used for sender auto coloring In this case
   * this are all tango colors without the grey tones 
   * See "http://tango.freedesktop.org/Tango_Icon_Theme_Guidelines" for details
   */
  // Butter
  addSenderAutoColor(SenderCol01, "#fce94f");
  addSenderAutoColor(SenderCol02, "#edd400");
  addSenderAutoColor(SenderCol03,  "#c4a000");
  // Orange
  addSenderAutoColor(SenderCol04,  "#fcaf3e");
  addSenderAutoColor(SenderCol05,  "#f57900");
  addSenderAutoColor(SenderCol06,  "#ce5c00");
  // Chocolate
  addSenderAutoColor(SenderCol07, "#e9b96e");
  addSenderAutoColor(SenderCol08, "#c17d11");
  addSenderAutoColor(SenderCol09, "#8f5902");
  // Chameleon
  addSenderAutoColor(SenderCol10, "#8ae234");
  addSenderAutoColor(SenderCol11, "#73d216");
  addSenderAutoColor(SenderCol12, "#4e9a06");
  // Sky Blue
  addSenderAutoColor(SenderCol13, "#729fcf");
  addSenderAutoColor(SenderCol14, "#3465a4");
  addSenderAutoColor(SenderCol15, "#204a87");
  // Plum
  addSenderAutoColor(SenderCol16, "#ad7fa8");
  addSenderAutoColor(SenderCol17, "#75507b");
  addSenderAutoColor(SenderCol18, "#5c3566");
  // Scarlet Red
  addSenderAutoColor(SenderCol19, "#ef2929");
  addSenderAutoColor(SenderCol20, "#cc0000");
  addSenderAutoColor(SenderCol21, "#a40000");

  QTextCharFormat nick;
  nick.setAnchor(true);
  nick.setFontWeight(QFont::Bold);
  setFormat(Nick, nick, Settings::Default);

  QTextCharFormat hostmask;
  hostmask.setFontItalic(true);
  setFormat(Hostmask, hostmask, Settings::Default);

  QTextCharFormat channel;
  channel.setAnchor(true);
  channel.setFontWeight(QFont::Bold);
  setFormat(ChannelName, channel, Settings::Default);

  QTextCharFormat flags;
  flags.setFontWeight(QFont::Bold);
  setFormat(ModeFlags, flags, Settings::Default);

  QTextCharFormat url;
  url.setFontUnderline(true);
  url.setAnchor(true);
  setFormat(Url, url, Settings::Default);

  QtUiStyleSettings s;
  _highlightColor = s.highlightColor();
  if(!_highlightColor.isValid()) _highlightColor = QColor("lightcoral");
}

QtUiStyle::~QtUiStyle() {}

void QtUiStyle::setHighlightColor(const QColor &col) {
  _highlightColor = col;
  QtUiStyleSettings s;
  s.setHighlightColor(col);
}

void QtUiStyle::addSenderAutoColor(FormatType type, const QString name) 
{
  QTextCharFormat autoColor;
  autoColor.setAnchor(true);
  autoColor.setForeground(QBrush(QColor(name)));
  setFormat(type, autoColor, Settings::Default);
}
