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
#include <QApplication>

#include "quassel.h"
#include "uistyle.h"
#include "uisettings.h"
#include "util.h"

UiStyle::UiStyle(const QString &settingsKey) : _settingsKey(settingsKey) {
  // register FormatList if that hasn't happened yet
  // FIXME I don't think this actually avoids double registration... then again... does it hurt?
  if(QVariant::nameToType("UiStyle::FormatList") == QVariant::Invalid) {
    qRegisterMetaType<FormatList>("UiStyle::FormatList");
    qRegisterMetaTypeStreamOperators<FormatList>("UiStyle::FormatList");
    Q_ASSERT(QVariant::nameToType("UiStyle::FormatList") != QVariant::Invalid);
  }

  _defaultFont = QFont("Monospace", QApplication::font().pointSize());

  // Default format
  _defaultPlainFormat.setForeground(QBrush("#000000"));
  _defaultPlainFormat.setFont(_defaultFont);
  _defaultPlainFormat.font().setFixedPitch(true);
  _defaultPlainFormat.font().setStyleHint(QFont::TypeWriter);
  setFormat(None, _defaultPlainFormat, Settings::Default);

  // Load saved custom formats
  UiStyleSettings s(_settingsKey);
  foreach(FormatType type, s.availableFormats()) {
    _customFormats[type] = s.customFormat(type);
  }

  // Check for the sender auto coloring option
  _senderAutoColor = s.value("Colors/SenderAutoColor", false).toBool();

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

  // Initialize color codes according to mIRC "standard"
  QStringList colors;
  //colors << "white" << "black" << "navy" << "green" << "red" << "maroon" << "purple" << "orange";
  //colors << "yellow" << "lime" << "teal" << "aqua" << "royalblue" << "fuchsia" << "grey" << "silver";
  colors << "#ffffff" << "#000000" << "#000080" << "#008000" << "#ff0000" << "#800000" << "#800080" << "#ffa500";
  colors << "#ffff00" << "#00ff00" << "#008080" << "#00ffff" << "#4169E1" << "#ff00ff" << "#808080" << "#c0c0c0";

  // Set color formats
  for(int i = 0; i < 16; i++) {
    QString idx = QString("%1").arg(i, (int)2, (int)10, (QChar)'0');
    _formatCodes[QString("%Dcf%1").arg(idx)] = (FormatType)(FgCol00 | i<<24);
    _formatCodes[QString("%Dcb%1").arg(idx)] = (FormatType)(BgCol00 | i<<28);
    QTextCharFormat fgf, bgf;
    fgf.setForeground(QBrush(QColor(colors[i]))); setFormat((FormatType)(FgCol00 | i<<24), fgf, Settings::Default);
    bgf.setBackground(QBrush(QColor(colors[i]))); setFormat((FormatType)(BgCol00 | i<<28), bgf, Settings::Default);
  }

  // Set a few more standard formats
  QTextCharFormat bold; bold.setFontWeight(QFont::Bold);
  setFormat(Bold, bold, Settings::Default);

  QTextCharFormat italic; italic.setFontItalic(true);
  setFormat(Italic, italic, Settings::Default);

  QTextCharFormat underline; underline.setFontUnderline(true);
  setFormat(Underline, underline, Settings::Default);

  loadStyleSheet();
  // All other formats should be defined in derived classes.
}

UiStyle::~ UiStyle() {
  qDeleteAll(_cachedFontMetrics);
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
      _customFormats.remove(ftype);
      s.removeCustomFormat(ftype);
    }
  }
  // TODO: invalidate only affected cached formats... if that's possible with less overhead than just rebuilding them
  _cachedFormats.clear();
  _cachedFontMetrics.clear();
}

void UiStyle::setSenderAutoColor( bool state ) {
  _senderAutoColor = state;
  UiStyleSettings s(_settingsKey);
  s.setValue("Colors/SenderAutoColor", QVariant(state));
}

QTextCharFormat UiStyle::format(FormatType ftype, Settings::Mode mode) const {
  // Check for enabled sender auto coloring
  if ( (ftype & 0x00000fff) == Sender && !_senderAutoColor ) {
    // Just use the default sender style if auto coloring is disabled FIXME
    ftype = Sender;
  }

  if(mode == Settings::Custom && _customFormats.contains(ftype)) return _customFormats.value(ftype);
  else return _defaultFormats.value(ftype, QTextCharFormat());
}

// NOTE: This function is intimately tied to the values in FormatType. Don't change this
//       until you _really_ know what you do!
QTextCharFormat UiStyle::mergedFormat(quint32 ftype) {
  if(ftype == Invalid)
    return QTextCharFormat();
  if(_cachedFormats.contains(ftype))
    return _cachedFormats.value(ftype);

  QTextCharFormat fmt;

  // Check if we have a special message format stored already sans color codes
  if(_cachedFormats.contains(ftype & 0x1fffff))
    fmt = _cachedFormats.value(ftype &0x1fffff);
  else {
    // Nope, so we have to construct it...
    // We assume that we don't mix mirc format codes and component codes, so at this point we know that it's either
    // a stock (not stylesheeted) component, or mirc formats
    // In both cases, we start off with the basic format for this message type and then merge the extra stuff

    // Check if we at least already have something stored for the message type first
    if(_cachedFormats.contains(ftype & 0xf))
      fmt = _cachedFormats.value(ftype & 0xf);
    else {
      // Not being in the cache means it hasn't been preset via stylesheet (or used before)
      // We might still have set something in code as a fallback, so merge
      fmt = format(None);
      fmt.merge(format((FormatType)(ftype & 0x0f)));
      // This can be cached
      _cachedFormats[ftype & 0x0f] = fmt;
    }
    // OK, at this point we have the message type format - now merge the rest
    if((ftype & 0xf0)) { // mirc format
      for(quint32 mask = 0x10; mask <= 0x80; mask <<= 1) {
        if(!(ftype & mask))
          continue;
        // We need to check for overrides in the cache
        if(_cachedFormats.contains(mask | (ftype & 0x0f)))
          fmt.merge(_cachedFormats.value(mask | (ftype & 0x0f)));
        else if(_cachedFormats.contains(mask))
          fmt.merge(_cachedFormats.value(mask));
        else // nothing in cache, use stock format
          fmt.merge(format((FormatType)mask));
      }
    } else { // component
      // We've already checked the cache for the combo of msgtype and component and failed,
      // so we check if we defined a general format and merge this, or the stock format else
      if(_cachedFormats.contains(ftype & 0xff00))
        fmt.merge(_cachedFormats.value(ftype & 0xff00));
      else
        fmt.merge(format((FormatType)(ftype & 0xff00)));
    }
  }

  // Now we handle color codes. We assume that those can't be combined with components
  if(ftype & 0x00400000)
    fmt.merge(format((FormatType)(ftype & 0x0f400000))); // foreground
  if(ftype & 0x00800000)
    fmt.merge(format((FormatType)(ftype & 0xf0800000))); // background

  // Sender auto colors
  if(_senderAutoColor && (ftype & 0x200) && (ftype & 0xff000200) != 0x200) {
    if(_cachedFormats.contains(ftype & 0xff00020f))
      fmt.merge(_cachedFormats.value(ftype & 0xff00020f));
    else if(_cachedFormats.contains(ftype & 0xff000200))
      fmt.merge(_cachedFormats.value(ftype & 0xff000200));
    else
      fmt.merge(format((FormatType)(ftype & 0xff000200)));
  }

  // URL
  if(ftype & Url)
    fmt.merge(format(Url)); // TODO handle this properly

  return _cachedFormats[ftype] = fmt;
}

QFontMetricsF *UiStyle::fontMetrics(quint32 ftype) {
  // QFontMetricsF is not assignable, so we need to store pointers :/
  if(_cachedFontMetrics.contains(ftype)) return _cachedFontMetrics.value(ftype);
  return (_cachedFontMetrics[ftype] = new QFontMetricsF(mergedFormat(ftype).font()));
}

UiStyle::FormatType UiStyle::formatType(const QString & code) const {
  if(_formatCodes.contains(code)) return _formatCodes.value(code);
  return Invalid;
}

QString UiStyle::formatCode(FormatType ftype) const {
  return _formatCodes.key(ftype);
}

QList<QTextLayout::FormatRange> UiStyle::toTextLayoutList(const FormatList &formatList, int textLength) {
  QList<QTextLayout::FormatRange> formatRanges;
  QTextLayout::FormatRange range;
  int i = 0;
  for(i = 0; i < formatList.count(); i++) {
    range.format = mergedFormat(formatList.at(i).second);
    range.start = formatList.at(i).first;
    if(i > 0) formatRanges.last().length = range.start - formatRanges.last().start;
    formatRanges.append(range);
  }
  if(i > 0) formatRanges.last().length = textLength - formatRanges.last().start;
  return formatRanges;
}

// This method expects a well-formatted string, there is no error checking!
// Since we create those ourselves, we should be pretty safe that nobody does something crappy here.
UiStyle::StyledString UiStyle::styleString(const QString &s_) {
  QString s = s_;
  if(s.length() > 65535) {
    qWarning() << QString("String too long to be styled: %1").arg(s);
    return StyledString();
  }
  StyledString result;
  result.formatList.append(qMakePair((quint16)0, (quint32)None));
  quint32 curfmt = (quint32)None;
  int pos = 0; quint16 length = 0;
  for(;;) {
    pos = s.indexOf('%', pos);
    if(pos < 0) break;
    if(s[pos+1] == '%') { // escaped %, we just remove one and continue
      s.remove(pos, 1);
      pos++;
      continue;
    }
    if(s[pos+1] == 'D' && s[pos+2] == 'c') { // color code
      if(s[pos+3] == '-') {  // color off
        curfmt &= 0x003fffff;
        length = 4;
      } else {
        int color = 10 * s[pos+4].digitValue() + s[pos+5].digitValue();
        //TODO: use 99 as transparent color (re mirc color "standard")
        color &= 0x0f;
        if(s[pos+3] == 'f') {
          curfmt &= 0xf0ffffff;
          curfmt |= (color << 24) | 0x00400000;
        } else {
          curfmt &= 0x0fffffff;
          curfmt |= (color << 28) | 0x00800000;
        }
        length = 6;
      }
    } else if(s[pos+1] == 'O') { // reset formatting
      curfmt &= 0x0000000f; // we keep message type-specific formatting
      length = 2;
    } else if(s[pos+1] == 'R') { // reverse
      // TODO: implement reverse formatting

      length = 2;
    } else { // all others are toggles
      QString code = QString("%") + s[pos+1];
      if(s[pos+1] == 'D') code += s[pos+2];
      FormatType ftype = formatType(code);
      if(ftype == Invalid) {
        qWarning() << (QString("Invalid format code in string: %1").arg(s));
        continue;
      }
      curfmt ^= ftype;
      length = code.length();
    }
    s.remove(pos, length);
    if(pos == result.formatList.last().first)
      result.formatList.last().second = curfmt;
    else
      result.formatList.append(qMakePair((quint16)pos, curfmt));
  }
  result.plainText = s;
  return result;
}

QString UiStyle::mircToInternal(const QString &mirc_) const {
  QString mirc = mirc_;
  mirc.replace('%', "%%");      // escape % just to be sure
  mirc.replace('\x02', "%B");
  mirc.replace('\x0f', "%O");
  mirc.replace('\x12', "%R");
  mirc.replace('\x16', "%R");
  mirc.replace('\x1d', "%S");
  mirc.replace('\x1f', "%U");

  // Now we bring the color codes (\x03) in a sane format that can be parsed more easily later.
  // %Dcfxx is foreground, %Dcbxx is background color, where xx is a 2 digit dec number denoting the color code.
  // %Dc- turns color off.
  // Note: We use the "mirc standard" as described in <http://www.mirc.co.uk/help/color.txt>.
  //       This means that we don't accept something like \x03,5 (even though others, like WeeChat, do).
  int pos = 0;
  for(;;) {
    pos = mirc.indexOf('\x03', pos);
    if(pos < 0) break; // no more mirc color codes
    QString ins, num;
    int l = mirc.length();
    int i = pos + 1;
    // check for fg color
    if(i < l && mirc[i].isDigit()) {
      num = mirc[i++];
      if(i < l && mirc[i].isDigit()) num.append(mirc[i++]);
      else num.prepend('0');
      ins = QString("%Dcf%1").arg(num);

      if(i+1 < l && mirc[i] == ',' && mirc[i+1].isDigit()) {
        i++;
        num = mirc[i++];
        if(i < l && mirc[i].isDigit()) num.append(mirc[i++]);
        else num.prepend('0');
        ins += QString("%Dcb%1").arg(num);
      }
    } else {
      ins = "%Dc-";
    }
    mirc.replace(pos, i-pos, ins);
  }
  return mirc;
}

/***********************************************************************************/
UiStyle::StyledMessage::StyledMessage(const Message &msg)
  : Message(msg)
{
}

void UiStyle::StyledMessage::style(UiStyle *style) const {
  QString user = userFromMask(sender());
  QString host = hostFromMask(sender());
  QString nick = nickFromMask(sender());
  QString txt = style->mircToInternal(contents());
  QString bufferName = bufferInfo().bufferName();
  bufferName.replace('%', "%%"); // well, you _can_ have a % in a buffername apparently... -_-

  QString t;
  switch(type()) {
    case Message::Plain:
      t = tr("%D0%1").arg(txt); break;
    case Message::Notice:
      t = tr("%Dn%1").arg(txt); break;
    case Message::Topic:
    case Message::Server:
      t = tr("%Ds%1").arg(txt); break;
    case Message::Error:
      t = tr("%De%1").arg(txt); break;
    case Message::Join:
      t = tr("%Dj%DN%1%DN %DH(%2@%3)%DH has joined %DC%4%DC").arg(nick, user, host, bufferName); break;
    case Message::Part:
      t = tr("%Dp%DN%1%DN %DH(%2@%3)%DH has left %DC%4%DC").arg(nick, user, host, bufferName);
      if(!txt.isEmpty()) t = QString("%1 (%2)").arg(t).arg(txt);
      break;
    case Message::Quit:
      t = tr("%Dq%DN%1%DN %DH(%2@%3)%DH has quit").arg(nick, user, host);
      if(!txt.isEmpty()) t = QString("%1 (%2)").arg(t).arg(txt);
      break;
    case Message::Kick: {
        QString victim = txt.section(" ", 0, 0);
        QString kickmsg = txt.section(" ", 1);
        t = tr("%Dk%DN%1%DN has kicked %DN%2%DN from %DC%3%DC").arg(nick).arg(victim).arg(bufferName);
        if(!kickmsg.isEmpty()) t = QString("%1 (%2)").arg(t).arg(kickmsg);
      }
      break;
    case Message::Nick:
      if(nick == contents()) t = tr("%DrYou are now known as %DN%1%DN").arg(txt);
      else t = tr("%Dr%DN%1%DN is now known as %DN%2%DN").arg(nick, txt);
      break;
    case Message::Mode:
      if(nick.isEmpty()) t = tr("%DmUser mode: %DM%1%DM").arg(txt);
      else t = tr("%DmMode %DM%1%DM by %DN%2%DN").arg(txt, nick);
      break;
    case Message::Action:
      t = tr("%Da%DN%1%DN %2").arg(nick).arg(txt);
      break;
    default:
      t = tr("%De[%1]").arg(txt);
  }
  _contents = style->styleString(t);
}

QString UiStyle::StyledMessage::decoratedTimestamp() const {
  return QString("[%1]").arg(timestamp().toLocalTime().toString("hh:mm:ss"));
}

QString UiStyle::StyledMessage::plainSender() const {
  switch(type()) {
    case Message::Plain:
    case Message::Notice:
      return nickFromMask(sender());
    default:
      return QString();
  }
}

QString UiStyle::StyledMessage::decoratedSender() const {
  switch(type()) {
    case Message::Plain:
      return tr("<%1>").arg(plainSender()); break;
    case Message::Notice:
      return tr("[%1]").arg(plainSender()); break;
    case Message::Topic:
    case Message::Server:
      return tr("*"); break;
    case Message::Error:
      return tr("*"); break;
    case Message::Join:
      return tr("-->"); break;
    case Message::Part:
      return tr("<--"); break;
    case Message::Quit:
      return tr("<--"); break;
    case Message::Kick:
      return tr("<-*"); break;
    case Message::Nick:
      return tr("<->"); break;
    case Message::Mode:
      return tr("***"); break;
    case Message::Action:
      return tr("-*-"); break;
    default:
      return tr("%1").arg(plainSender());
  }
}

UiStyle::FormatType UiStyle::StyledMessage::senderFormat() const {
  switch(type()) {
    case Message::Plain:
      // To produce random like but stable nick colorings some sort of hashing should work best.
      // In this case we just use the qt function qChecksum which produces a
      // CRC16 hash. This should be fast and 16 bits are more than enough.
      {
        QString nick = nickFromMask(sender()).toLower();
        if(!nick.isEmpty()) {
          int chopCount = 0;
          while(nick[nick.count() - 1 - chopCount] == '_') {
            chopCount++;
          }
          nick.chop(chopCount);
        }
        quint16 hash = qChecksum(nick.toAscii().data(), nick.toAscii().size());
        return (UiStyle::FormatType)((((hash % 12) + 1) << 24) + 0x200); // FIXME: amount of sender colors hardwired
      }
    case Message::Notice:
      return UiStyle::NoticeMsg; break;
    case Message::Topic:
    case Message::Server:
      return UiStyle::ServerMsg; break;
    case Message::Error:
      return UiStyle::ErrorMsg; break;
    case Message::Join:
      return UiStyle::JoinMsg; break;
    case Message::Part:
      return UiStyle::PartMsg; break;
    case Message::Quit:
      return UiStyle::QuitMsg; break;
    case Message::Kick:
      return UiStyle::KickMsg; break;
    case Message::Nick:
      return UiStyle::RenameMsg; break;
    case Message::Mode:
      return UiStyle::ModeMsg; break;
    case Message::Action:
      return UiStyle::ActionMsg; break;
    default:
      return UiStyle::ErrorMsg;
  }
}


/***********************************************************************************/

QDataStream &operator<<(QDataStream &out, const UiStyle::FormatList &formatList) {
  out << formatList.count();
  UiStyle::FormatList::const_iterator it = formatList.begin();
  while(it != formatList.end()) {
    out << (*it).first << (*it).second;
    ++it;
  }
  return out;
}

QDataStream &operator>>(QDataStream &in, UiStyle::FormatList &formatList) {
  quint16 cnt;
  in >> cnt;
  for(quint16 i = 0; i < cnt; i++) {
    quint16 pos; quint32 ftype;
    in >> pos >> ftype;
    formatList.append(qMakePair((quint16)pos, ftype));
  }
  return in;
}

/***********************************************************************************/
// Stylesheet handling
/***********************************************************************************/

void UiStyle::loadStyleSheet() {
  QssParser parser;
  parser.loadStyleSheet(qApp->styleSheet());

  // TODO handle results
  QApplication::setPalette(parser.palette());
}

UiStyle::QssParser::QssParser() {
  _palette = QApplication::palette();

  // Init palette color roles
  _paletteColorRoles["alternate-base"] = QPalette::AlternateBase;
  _paletteColorRoles["background"] = QPalette::Background;
  _paletteColorRoles["base"] = QPalette::Base;
  _paletteColorRoles["bright-text"] = QPalette::BrightText;
  _paletteColorRoles["button"] = QPalette::Button;
  _paletteColorRoles["button-text"] = QPalette::ButtonText;
  _paletteColorRoles["dark"] = QPalette::Dark;
  _paletteColorRoles["foreground"] = QPalette::Foreground;
  _paletteColorRoles["highlight"] = QPalette::Highlight;
  _paletteColorRoles["highlighted-text"] = QPalette::HighlightedText;
  _paletteColorRoles["light"] = QPalette::Light;
  _paletteColorRoles["link"] = QPalette::Link;
  _paletteColorRoles["link-visited"] = QPalette::LinkVisited;
  _paletteColorRoles["mid"] = QPalette::Mid;
  _paletteColorRoles["midlight"] = QPalette::Midlight;
  _paletteColorRoles["shadow"] = QPalette::Shadow;
  _paletteColorRoles["text"] = QPalette::Text;
  _paletteColorRoles["tooltip-base"] = QPalette::ToolTipBase;
  _paletteColorRoles["tooltip-text"] = QPalette::ToolTipText;
  _paletteColorRoles["window"] = QPalette::Window;
  _paletteColorRoles["window-text"] = QPalette::WindowText;
}

void UiStyle::QssParser::loadStyleSheet(const QString &styleSheet) {
  QString ss = styleSheet;
  ss = "file:////home/sputnick/devel/quassel/test.qss"; // FIXME
  if(ss.startsWith("file:///")) {
    ss.remove(0, 8);
    QFile file(ss);
    if(file.open(QFile::ReadOnly)) {
      QTextStream stream(&file);
      ss = stream.readAll();
    } else {
      qWarning() << tr("Could not read stylesheet \"%1\"!").arg(file.fileName());
      return;
    }
  }
  if(ss.isEmpty())
    return;

  // Now we have the stylesheet itself in ss, start parsing
  QRegExp blockrx("((ChatLine|Palette)[^{]*)\\{([^}]+)\\}");
  int pos = 0;
  while((pos = blockrx.indexIn(ss, pos)) >= 0) {
    //qDebug() << blockrx.cap(1) << blockrx.cap(3);

    if(blockrx.cap(2) == "ChatLine")
      parseChatLineData(blockrx.cap(1).trimmed(), blockrx.cap(3).trimmed());
    else
      parsePaletteData(blockrx.cap(1).trimmed(), blockrx.cap(3).trimmed());

    pos += blockrx.matchedLength();
  }

}

void UiStyle::QssParser::parseChatLineData(const QString &decl, const QString &contents) {


}

// Palette { ... } specifies the application palette
// ColorGroups can be specified like pseudo states, chaining is OR (contrary to normal CSS handling):
//   Palette:inactive:disabled { ... } applies to both the Inactive and the Disabled state
void UiStyle::QssParser::parsePaletteData(const QString &decl, const QString &contents) {
  QList<QPalette::ColorGroup> colorGroups;

  // Check if we want to apply this palette definition for particular ColorGroups
  QRegExp rx("Palette((:(normal|active|inactive|disabled))*)");
  if(!rx.exactMatch(decl)) {
    qWarning() << tr("Invalid block declaration: %1").arg(decl);
    return;
  }
  if(!rx.cap(1).isEmpty()) {
    QStringList groups = rx.cap(1).split(':', QString::SkipEmptyParts);
    foreach(QString g, groups) {
      if((g == "normal" || g == "active") && !colorGroups.contains(QPalette::Active))
        colorGroups.append(QPalette::Active);
      else if(g == "inactive" && !colorGroups.contains(QPalette::Inactive))
        colorGroups.append(QPalette::Inactive);
      else if(g == "disabled" && !colorGroups.contains(QPalette::Disabled))
        colorGroups.append(QPalette::Disabled);
    }
  }

  // Now let's go through the roles
  foreach(QString line, contents.split(';', QString::SkipEmptyParts)) {
    int idx = line.indexOf(':');
    if(idx <= 0) {
      qWarning() << tr("Invalid palette role assignment: %1").arg(line.trimmed());
      continue;
    }
    QString rolestr = line.left(idx).trimmed();
    QString brushstr = line.mid(idx + 1).trimmed();
    if(!_paletteColorRoles.contains(rolestr)) {
      qWarning() << tr("Unknown palette role name: %1").arg(rolestr);
      continue;
    }
    QBrush brush = parseBrushValue(brushstr);
    if(colorGroups.count()) {
      foreach(QPalette::ColorGroup group, colorGroups)
        _palette.setBrush(group, _paletteColorRoles.value(rolestr), brush);
    } else
      _palette.setBrush(_paletteColorRoles.value(rolestr), brush);
  }
}

QBrush UiStyle::QssParser::parseBrushValue(const QString &str) {
  QColor c = parseColorValue(str);
  if(c.isValid())
    return QBrush(c);

  if(str.startsWith("palette")) { // Palette color role
    QRegExp rx("palette\\s*\\(\\s*([a-z-]+)\\s*\\)");
    if(!rx.exactMatch(str)) {
      qWarning() << tr("Invalid palette color role specification: %1").arg(str);
      return QBrush();
    }
    if(!_paletteColorRoles.contains(rx.cap(1))) {
      qWarning() << tr("Unknown palette color role: %1").arg(rx.cap(1));
      return QBrush();
    }
    return QBrush(_palette.brush(_paletteColorRoles.value(rx.cap(1))));

  } else if(str.startsWith("qlineargradient")) {
    static QString rxFloat("\\s*(-?\\s*[0-9]*\\.?[0-9]+)\\s*");
    QRegExp rx(QString("qlineargradient\\s*\\(\\s*x1:%1,\\s*y1:%1,\\s*x2:%1,\\s*y2:%1,(.+)\\)").arg(rxFloat));
    if(!rx.exactMatch(str)) {
      qWarning() << tr("Invalid gradient declaration: %1").arg(str);
      return QBrush();
    }
    qreal x1 = rx.cap(1).toDouble();
    qreal y1 = rx.cap(2).toDouble();
    qreal x2 = rx.cap(3).toDouble();
    qreal y2 = rx.cap(4).toDouble();
    QGradientStops stops = parseGradientStops(rx.cap(5).trimmed());
    if(!stops.count()) {
      qWarning() << tr("Invalid gradient stops list: %1").arg(str);
      return QBrush();
    }
    QLinearGradient gradient(x1, y1, x2, y2);
    gradient.setStops(stops);
    return QBrush(gradient);

  } else if(str.startsWith("qconicalgradient")) {
    static QString rxFloat("\\s*(-?\\s*[0-9]*\\.?[0-9]+)\\s*");
    QRegExp rx(QString("qconicalgradient\\s*\\(\\s*cx:%1,\\s*cy:%1,\\s*angle:%1,(.+)\\)").arg(rxFloat));
    if(!rx.exactMatch(str)) {
      qWarning() << tr("Invalid gradient declaration: %1").arg(str);
      return QBrush();
    }
    qreal cx = rx.cap(1).toDouble();
    qreal cy = rx.cap(2).toDouble();
    qreal angle = rx.cap(3).toDouble();
    QGradientStops stops = parseGradientStops(rx.cap(4).trimmed());
    if(!stops.count()) {
      qWarning() << tr("Invalid gradient stops list: %1").arg(str);
      return QBrush();
    }
    QConicalGradient gradient(cx, cy, angle);
    gradient.setStops(stops);
    return QBrush(gradient);

  } else if(str.startsWith("qradialgradient")) {
    static QString rxFloat("\\s*(-?\\s*[0-9]*\\.?[0-9]+)\\s*");
    QRegExp rx(QString("qradialgradient\\s*\\(\\s*cx:%1,\\s*cy:%1,\\s*radius:%1,\\s*fx:%1,\\s*fy:%1,(.+)\\)").arg(rxFloat));
    if(!rx.exactMatch(str)) {
      qWarning() << tr("Invalid gradient declaration: %1").arg(str);
      return QBrush();
    }
    qreal cx = rx.cap(1).toDouble();
    qreal cy = rx.cap(2).toDouble();
    qreal radius = rx.cap(3).toDouble();
    qreal fx = rx.cap(4).toDouble();
    qreal fy = rx.cap(5).toDouble();
    QGradientStops stops = parseGradientStops(rx.cap(6).trimmed());
    if(!stops.count()) {
      qWarning() << tr("Invalid gradient stops list: %1").arg(str);
      return QBrush();
    }
    QRadialGradient gradient(cx, cy, radius, fx, fy);
    gradient.setStops(stops);
    return QBrush(gradient);
  }

  return QBrush();
}

QColor UiStyle::QssParser::parseColorValue(const QString &str) {
  if(str.startsWith("rgba")) {
    ColorTuple tuple = parseColorTuple(str.mid(4));
    if(tuple.count() == 4)
      return QColor(tuple.at(0), tuple.at(1), tuple.at(2), tuple.at(3));
  } else if(str.startsWith("rgb")) {
    ColorTuple tuple = parseColorTuple(str.mid(3));
    if(tuple.count() == 3)
      return QColor(tuple.at(0), tuple.at(1), tuple.at(2));
  } else if(str.startsWith("hsva")) {
    ColorTuple tuple = parseColorTuple(str.mid(4));
    if(tuple.count() == 4) {
      QColor c;
      c.setHsvF(tuple.at(0), tuple.at(1), tuple.at(2), tuple.at(3));
      return c;
    }
  } else if(str.startsWith("hsv")) {
    ColorTuple tuple = parseColorTuple(str.mid(3));
    if(tuple.count() == 3) {
      QColor c;
      c.setHsvF(tuple.at(0), tuple.at(1), tuple.at(2));
      return c;
    }
  } else {
    QRegExp rx("#?[0-9A-Fa-z]+");
    if(rx.exactMatch(str))
      return QColor(str);
  }
  return QColor();
}

// get a list of comma-separated int values or percentages (rel to 0-255)
UiStyle::QssParser::ColorTuple UiStyle::QssParser::parseColorTuple(const QString &str) {
  ColorTuple result;
  QRegExp rx("\\(((\\s*[0-9]{1,3}%?\\s*)(,\\s*[0-9]{1,3}%?\\s*)*)\\)");
  if(!rx.exactMatch(str.trimmed())) {
    return ColorTuple();
  }
  QStringList values = rx.cap(1).split(',');
  foreach(QString v, values) {
    qreal val;
    bool perc = false;
    bool ok;
    v = v.trimmed();
    if(v.endsWith('%')) {
      perc = true;
      v.chop(1);
    }
    val = (qreal)v.toUInt(&ok);
    if(!ok)
      return ColorTuple();
    if(perc)
      val = 255 * val/100;
    result.append(val);
  }
  return result;
}

QGradientStops UiStyle::QssParser::parseGradientStops(const QString &str_) {
  QString str = str_;
  QGradientStops result;
  static QString rxFloat("(0?\\.[0-9]+|[01])"); // values between 0 and 1
  QRegExp rx(QString("\\s*,?\\s*stop:\\s*(%1)\\s+([^:]+)(,\\s*stop:|$)").arg(rxFloat));
  int idx;
  while((idx = rx.indexIn(str)) == 0) {
    qreal x = rx.cap(1).toDouble();
    QColor c = parseColorValue(rx.cap(3));
    if(!c.isValid())
      return QGradientStops();
    result << QGradientStop(x, c);
    str.remove(0, rx.matchedLength() - rx.cap(4).length());
  }
  if(!str.trimmed().isEmpty())
    return QGradientStops();

  return result;
}
