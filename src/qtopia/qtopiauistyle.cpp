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

#include "qtopiauistyle.h"
#include "settings.h"

QtopiaUiStyle::QtopiaUiStyle() : UiStyle("QtopiaUiStyle") {

  QTextCharFormat def;
  def.setForeground(QBrush("#000000"));
  def.setFont(QFont("Verdana",5));
  setFormat(None, def, Settings::Default);

  // We need to just set our internal formats; everything else is done by the base class...

  // Internal message formats

  QTextCharFormat plainMsg;
  plainMsg.setForeground(QBrush("#000000"));
  setFormat(PlainMsg, plainMsg, Settings::Default);

  QTextCharFormat notice;
  notice.setForeground(QBrush("#000080"));
  setFormat(NoticeMsg, notice, Settings::Default);

  QTextCharFormat server;
  server.setForeground(QBrush("#000080"));
  setFormat(ServerMsg, server, Settings::Default);

  QTextCharFormat error;
  error.setForeground(QBrush("#ff0000"));
  setFormat(ErrorMsg, error, Settings::Default);

  QTextCharFormat join;
  join.setForeground(QBrush("#008000"));
  setFormat(JoinMsg, join, Settings::Default);

  QTextCharFormat part;
  part.setForeground(QBrush("#ff0000"));
  setFormat(PartMsg, part, Settings::Default);

  QTextCharFormat quit;
  quit.setForeground(QBrush("#ff0000"));
  setFormat(QuitMsg, quit, Settings::Default);

  QTextCharFormat kick;
  kick.setForeground(QBrush("#ff0000"));
  setFormat(KickMsg, kick, Settings::Default);

  QTextCharFormat nren;
  nren.setForeground(QBrush("#6a5acd"));
  setFormat(RenameMsg, nren, Settings::Default);

  QTextCharFormat mode;
  mode.setForeground(QBrush("#4682b4"));
  setFormat(ModeMsg, mode, Settings::Default);

  QTextCharFormat action;
  action.setFontItalic(true);
  action.setForeground(QBrush("#8b008b"));
  setFormat(ActionMsg, action, Settings::Default);

  // Internal message element formats
  QTextCharFormat ts;
  ts.setForeground(QBrush("#808080"));
  setFormat(Timestamp, ts, Settings::Default);

  QTextCharFormat sender;
  sender.setAnchor(true);
  sender.setForeground(QBrush("#000080"));
  setFormat(Sender, sender, Settings::Default);

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

}

QtopiaUiStyle::~QtopiaUiStyle() {}
