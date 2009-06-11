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

#include "qssparser.h"

QssParser::QssParser()
: _maxSenderHash(0)
{
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

void QssParser::loadStyleSheet(const QString &styleSheet) {
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
  // Palette definitions first, so we can apply roles later on
  QRegExp paletterx("(Palette[^{]*)\\{([^}]+)\\}");
  int pos = 0;
  while((pos = paletterx.indexIn(ss, pos)) >= 0) {
    parsePaletteData(paletterx.cap(1).trimmed(), paletterx.cap(2).trimmed());
    pos += paletterx.matchedLength();
  }

  // Now we can parse the rest of our custom blocks
  QRegExp blockrx("((?:ChatLine|BufferList|NickList|TreeView)[^{]*)\\{([^}]+)\\}");
  pos = 0;
  while((pos = blockrx.indexIn(ss, pos)) >= 0) {
    //qDebug() << blockrx.cap(1) << blockrx.cap(2);

    if(blockrx.cap(1).startsWith("ChatLine"))
      parseChatLineData(blockrx.cap(1).trimmed(), blockrx.cap(2).trimmed());
    //else
    // TODO: add moar here

    pos += blockrx.matchedLength();
  }

}

void QssParser::parseChatLineData(const QString &decl, const QString &contents) {
  quint64 fmtType = parseFormatType(decl);
  if(fmtType == UiStyle::Invalid)
    return;

  QTextCharFormat format;

  foreach(QString line, contents.split(';', QString::SkipEmptyParts)) {
    int idx = line.indexOf(':');
    if(idx <= 0) {
      qWarning() << Q_FUNC_INFO << tr("Invalid property declaration: %1").arg(line.trimmed());
      continue;
    }
    QString property = line.left(idx).trimmed();
    QString value = line.mid(idx + 1).trimmed();

    if(property == "background" || property == "background-color")
      format.setBackground(parseBrushValue(value));
    else if(property == "foreground" || property == "color")
      format.setForeground(parseBrushValue(value));

    else {
      qWarning() << Q_FUNC_INFO << tr("Unknown ChatLine property: %1").arg(property);
    }
  }

  _formats[fmtType] = format;
}

quint64 QssParser::parseFormatType(const QString &decl) {
  QRegExp rx("ChatLine(?:::(\\w+))?(?:#(\\w+))?(?:\\[([=-,\\\"\\w\\s]+)\\])?\\s*");
  // $1: subelement; $2: msgtype; $3: conditionals
  if(!rx.exactMatch(decl)) {
    qWarning() << Q_FUNC_INFO << tr("Invalid block declaration: %1").arg(decl);
    return UiStyle::Invalid;
  }
  QString subElement = rx.cap(1);
  QString msgType = rx.cap(2);
  QString conditions = rx.cap(3);

  quint64 fmtType = 0;

  // First determine the subelement
  if(!subElement.isEmpty()) {
    if(subElement == "timestamp")
      fmtType |= UiStyle::Timestamp;
    else if(subElement == "sender")
      fmtType |= UiStyle::Sender;
    else if(subElement == "nick")
      fmtType |= UiStyle::Nick;
    else if(subElement == "hostmask")
      fmtType |= UiStyle::Hostmask;
    else if(subElement == "modeflags")
      fmtType |= UiStyle::ModeFlags;
    else {
      qWarning() << Q_FUNC_INFO << tr("Invalid subelement name in %1").arg(decl);
      return UiStyle::Invalid;
    }
  }

  // Now, figure out the message type
  if(!msgType.isEmpty()) {
    if(msgType == "plain")
      fmtType |= UiStyle::PlainMsg;
    else if(msgType == "notice")
      fmtType |= UiStyle::NoticeMsg;
    else if(msgType == "server")
      fmtType |= UiStyle::ServerMsg;
    else if(msgType == "error")
      fmtType |= UiStyle::ErrorMsg;
    else if(msgType == "join")
      fmtType |= UiStyle::JoinMsg;
    else if(msgType == "part")
      fmtType |= UiStyle::PartMsg;
    else if(msgType == "quit")
      fmtType |= UiStyle::QuitMsg;
    else if(msgType == "kick")
      fmtType |= UiStyle::KickMsg;
    else if(msgType == "rename")
      fmtType |= UiStyle::RenameMsg;
    else if(msgType == "mode")
      fmtType |= UiStyle::ModeMsg;
    else if(msgType == "action")
      fmtType |= UiStyle::ActionMsg;
    else {
      qWarning() << Q_FUNC_INFO << tr("Invalid message type in %1").arg(decl);
    }
  }

  // Next up: conditional (formats, labels, nickhash)
  QRegExp condRx("\\s*(\\w+)\\s*=\\s*\"(\\w+)\"\\s*");
  if(!conditions.isEmpty()) {
    foreach(const QString &cond, conditions.split(',', QString::SkipEmptyParts)) {
      if(!condRx.exactMatch(cond)) {
        qWarning() << Q_FUNC_INFO << tr("Invalid condition %1").arg(cond);
        return UiStyle::Invalid;
      }
      QString condName = condRx.cap(1);
      QString condValue = condRx.cap(2);
      if(condName == "label") {
        quint64 labeltype = 0;
        if(condValue == "highlight")
          labeltype = UiStyle::Highlight;
        else {
          qWarning() << Q_FUNC_INFO << tr("Invalid message label: %1").arg(condValue);
          return UiStyle::Invalid;
        }
        fmtType |= (labeltype << 32);
      } else if(condName == "sender") {
        if(condValue == "self")
          fmtType |= (quint64)UiStyle::OwnMsg << 32; // sender="self" is actually treated as a label
          else {
            bool ok = true;
            quint64 val = condValue.toUInt(&ok, 16);
            if(!ok) {
              qWarning() << Q_FUNC_INFO << tr("Invalid senderhash specification: %1").arg(condValue);
              return UiStyle::Invalid;
            }
            if(val >= 255) {
              qWarning() << Q_FUNC_INFO << tr("Senderhash can be at most \"fe\"!");
              return UiStyle::Invalid;
            }
            fmtType |= val << 48;
          }
      }
      // TODO: colors
    }
  }

  return fmtType;
}

// Palette { ... } specifies the application palette
// ColorGroups can be specified like pseudo states, chaining is OR (contrary to normal CSS handling):
//   Palette:inactive:disabled { ... } applies to both the Inactive and the Disabled state
void QssParser::parsePaletteData(const QString &decl, const QString &contents) {
  QList<QPalette::ColorGroup> colorGroups;

  // Check if we want to apply this palette definition for particular ColorGroups
  QRegExp rx("Palette((:(normal|active|inactive|disabled))*)");
  if(!rx.exactMatch(decl)) {
    qWarning() << Q_FUNC_INFO << tr("Invalid block declaration: %1").arg(decl);
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
      qWarning() << Q_FUNC_INFO << tr("Invalid palette role assignment: %1").arg(line.trimmed());
      continue;
    }
    QString rolestr = line.left(idx).trimmed();
    QString brushstr = line.mid(idx + 1).trimmed();
    if(!_paletteColorRoles.contains(rolestr)) {
      qWarning() << Q_FUNC_INFO << tr("Unknown palette role name: %1").arg(rolestr);
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

QBrush QssParser::parseBrushValue(const QString &str, bool *ok) {
  if(ok)
    *ok = false;
  QColor c = parseColorValue(str);
  if(c.isValid()) {
    if(ok)
      *ok = true;
    return QBrush(c);
  }

  if(str.startsWith("palette")) { // Palette color role
    QRegExp rx("palette\\s*\\(\\s*([a-z-]+)\\s*\\)");
    if(!rx.exactMatch(str)) {
      qWarning() << Q_FUNC_INFO << tr("Invalid palette color role specification: %1").arg(str);
      return QBrush();
    }
    if(!_paletteColorRoles.contains(rx.cap(1))) {
      qWarning() << Q_FUNC_INFO << tr("Unknown palette color role: %1").arg(rx.cap(1));
      return QBrush();
    }
    return QBrush(_palette.brush(_paletteColorRoles.value(rx.cap(1))));

  } else if(str.startsWith("qlineargradient")) {
    static QString rxFloat("\\s*(-?\\s*[0-9]*\\.?[0-9]+)\\s*");
    QRegExp rx(QString("qlineargradient\\s*\\(\\s*x1:%1,\\s*y1:%1,\\s*x2:%1,\\s*y2:%1,(.+)\\)").arg(rxFloat));
    if(!rx.exactMatch(str)) {
      qWarning() << Q_FUNC_INFO << tr("Invalid gradient declaration: %1").arg(str);
      return QBrush();
    }
    qreal x1 = rx.cap(1).toDouble();
    qreal y1 = rx.cap(2).toDouble();
    qreal x2 = rx.cap(3).toDouble();
    qreal y2 = rx.cap(4).toDouble();
    QGradientStops stops = parseGradientStops(rx.cap(5).trimmed());
    if(!stops.count()) {
      qWarning() << Q_FUNC_INFO << tr("Invalid gradient stops list: %1").arg(str);
      return QBrush();
    }
    QLinearGradient gradient(x1, y1, x2, y2);
    gradient.setStops(stops);
    if(ok)
      *ok = true;
    return QBrush(gradient);

  } else if(str.startsWith("qconicalgradient")) {
    static QString rxFloat("\\s*(-?\\s*[0-9]*\\.?[0-9]+)\\s*");
    QRegExp rx(QString("qconicalgradient\\s*\\(\\s*cx:%1,\\s*cy:%1,\\s*angle:%1,(.+)\\)").arg(rxFloat));
    if(!rx.exactMatch(str)) {
      qWarning() << Q_FUNC_INFO << tr("Invalid gradient declaration: %1").arg(str);
      return QBrush();
    }
    qreal cx = rx.cap(1).toDouble();
    qreal cy = rx.cap(2).toDouble();
    qreal angle = rx.cap(3).toDouble();
    QGradientStops stops = parseGradientStops(rx.cap(4).trimmed());
    if(!stops.count()) {
      qWarning() << Q_FUNC_INFO << tr("Invalid gradient stops list: %1").arg(str);
      return QBrush();
    }
    QConicalGradient gradient(cx, cy, angle);
    gradient.setStops(stops);
    if(ok)
      *ok = true;
    return QBrush(gradient);

  } else if(str.startsWith("qradialgradient")) {
    static QString rxFloat("\\s*(-?\\s*[0-9]*\\.?[0-9]+)\\s*");
    QRegExp rx(QString("qradialgradient\\s*\\(\\s*cx:%1,\\s*cy:%1,\\s*radius:%1,\\s*fx:%1,\\s*fy:%1,(.+)\\)").arg(rxFloat));
    if(!rx.exactMatch(str)) {
      qWarning() << Q_FUNC_INFO << tr("Invalid gradient declaration: %1").arg(str);
      return QBrush();
    }
    qreal cx = rx.cap(1).toDouble();
    qreal cy = rx.cap(2).toDouble();
    qreal radius = rx.cap(3).toDouble();
    qreal fx = rx.cap(4).toDouble();
    qreal fy = rx.cap(5).toDouble();
    QGradientStops stops = parseGradientStops(rx.cap(6).trimmed());
    if(!stops.count()) {
      qWarning() << Q_FUNC_INFO << tr("Invalid gradient stops list: %1").arg(str);
      return QBrush();
    }
    QRadialGradient gradient(cx, cy, radius, fx, fy);
    gradient.setStops(stops);
    if(ok)
      *ok = true;
    return QBrush(gradient);
  }

  return QBrush();
}

QColor QssParser::parseColorValue(const QString &str) {
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
QssParser::ColorTuple QssParser::parseColorTuple(const QString &str) {
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

QGradientStops QssParser::parseGradientStops(const QString &str_) {
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
