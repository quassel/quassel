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

#include "util.h"

#include <QCoreApplication>
#include <QDebug>
#include <QLibraryInfo>
#include <QTextCodec>
#include <QTranslator>

#include "quassel.h"

class QMetaMethod;

QString nickFromMask(QString mask) {
  return mask.section('!', 0, 0);
}

QString userFromMask(QString mask) {
  QString userhost = mask.section('!', 1);
  if(userhost.isEmpty()) return QString();
  return userhost.section('@', 0, 0);
}

QString hostFromMask(QString mask) {
  QString userhost = mask.section('!', 1);
  if(userhost.isEmpty()) return QString();
  return userhost.section('@', 1);
}

bool isChannelName(QString str) {
  return QString("#&!+").contains(str[0]);
}

QString stripFormatCodes(QString str) {
  str.remove(QRegExp("\x03(\\d\\d?(,\\d\\d?)?)?"));
  str.remove('\x02');
  str.remove('\x0f');
  str.remove('\x12');
  str.remove('\x16');
  str.remove('\x1d');
  str.remove('\x1f');
  return str;
}

QString decodeString(const QByteArray &input, QTextCodec *codec) {
  // First, we check if it's utf8. It is very improbable to encounter a string that looks like
  // valid utf8, but in fact is not. This means that if the input string passes as valid utf8, it
  // is safe to assume that it is.
  // Q_ASSERT(sizeof(const char) == sizeof(quint8));  // In God we trust...
  bool isUtf8 = true;
  int cnt = 0;
  for(int i = 0; i < input.size(); i++) {
    if(cnt) {
      // We check a part of a multibyte char. These need to be of the form 10yyyyyy.
      if((input[i] & 0xc0) != 0x80) { isUtf8 = false; break; }
      cnt--;
      continue;
    }
    if((input[i] & 0x80) == 0x00) continue; // 7 bit is always ok
    if((input[i] & 0xf8) == 0xf0) { cnt = 3; continue; }  // 4-byte char 11110xxx 10yyyyyy 10zzzzzz 10vvvvvv
    if((input[i] & 0xf0) == 0xe0) { cnt = 2; continue; }  // 3-byte char 1110xxxx 10yyyyyy 10zzzzzz
    if((input[i] & 0xe0) == 0xc0) { cnt = 1; continue; }  // 2-byte char 110xxxxx 10yyyyyy
    isUtf8 = false; break;  // 8 bit char, but not utf8!
  }
  if(isUtf8 && cnt == 0) {
    QString s = QString::fromUtf8(input);
    //qDebug() << "Detected utf8:" << s;
    return s;
  }
  //QTextCodec *codec = QTextCodec::codecForName(encoding.toAscii());
  if(!codec) return QString::fromAscii(input);
  return codec->toUnicode(input);
}

/* not needed anymore
void writeDataToDevice(QIODevice *dev, const QVariant &item) {
  QByteArray block;
  QDataStream out(&block, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_4_2);
  out << (quint32)0 << item;
  out.device()->seek(0);
  out << (quint32)(block.size() - sizeof(quint32));
  dev->write(block);
}

bool readDataFromDevice(QIODevice *dev, quint32 &blockSize, QVariant &item) {
  QDataStream in(dev);
  in.setVersion(QDataStream::Qt_4_2);

  if(blockSize == 0) {
    if(dev->bytesAvailable() < (int)sizeof(quint32)) return false;
    in >> blockSize;
  }
  if(dev->bytesAvailable() < blockSize) return false;
  in >> item;
  return true;
}
*/

uint editingDistance(const QString &s1, const QString &s2) {
  uint n = s1.size()+1;
  uint m = s2.size()+1;
  QVector< QVector< uint > >matrix(n,QVector<uint>(m,0));

  for(uint i = 0; i < n; i++)
    matrix[i][0] = i;

  for(uint i = 0; i < m; i++)
    matrix[0][i] = i;

  uint min;
  for(uint i = 1; i < n; i++) {
    for(uint j = 1; j < m; j++) {
      uint deleteChar = matrix[i-1][j] + 1;
      uint insertChar = matrix[i][j-1] + 1;

      if(deleteChar < insertChar)
	min = deleteChar;
      else
	min = insertChar;

      if(s1[i-1] == s2[j-1]) {
	uint inheritChar = matrix[i-1][j-1];
	if(inheritChar < min)
	  min = inheritChar;
      }

      matrix[i][j] = min;
    }
  }
  return matrix[n-1][m-1];
}

QDir quasselDir() {
  QString quasselDir;
  if(Quassel::isOptionSet("datadir")) {
    quasselDir = Quassel::optionValue("datadir");
  } else {
    // FIXME use QDesktopServices
#ifdef Q_OS_WIN32
    quasselDir = qgetenv("APPDATA") + "/quassel/";
#elif defined Q_WS_MAC
    quasselDir = QDir::homePath() + "/Library/Application Support/Quassel/";
#else
    quasselDir = QDir::homePath() + "/.quassel/";
#endif
  }

  QDir qDir(quasselDir);
  if(!qDir.exists(quasselDir)) {
    if(!qDir.mkpath(quasselDir)) {
      qCritical() << "Unable to create Quassel data directory:" << qPrintable(qDir.absolutePath());
    }
  }

  return qDir;
}

void loadTranslation(const QLocale &locale) {
  QTranslator *qtTranslator = QCoreApplication::instance()->findChild<QTranslator *>("QtTr");
  QTranslator *quasselTranslator = QCoreApplication::instance()->findChild<QTranslator *>("QuasselTr");

  if(!qtTranslator) {
    qtTranslator = new QTranslator(qApp);
    qtTranslator->setObjectName("QtTr");
    qApp->installTranslator(qtTranslator);
  }
  if(!quasselTranslator) {
    quasselTranslator = new QTranslator(qApp);
    quasselTranslator->setObjectName("QuasselTr");
    qApp->installTranslator(quasselTranslator);
  }

  QLocale::setDefault(locale);

  if(locale.language() == QLocale::C)
    return;

  bool success = qtTranslator->load(QString(":i18n/qt_%1").arg(locale.name()));
  if(!success)
    qtTranslator->load(QString("%2/qt_%1").arg(locale.name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath)));
  quasselTranslator->load(QString(":i18n/quassel_%1").arg(locale.name()));
}

QString secondsToString(int timeInSeconds) {
    QList< QPair<int, QString> > timeUnit;
    timeUnit.append(qMakePair(365*24*60*60, QCoreApplication::translate("Quassel::secondsToString()", "year")));
    timeUnit.append(qMakePair(24*60*60, QCoreApplication::translate("Quassel::secondsToString()", "day")));
    timeUnit.append(qMakePair(60*60, QCoreApplication::translate("Quassel::secondsToString()", "h")));
    timeUnit.append(qMakePair(60, QCoreApplication::translate("Quassel::secondsToString()", "min")));
    timeUnit.append(qMakePair(1, QCoreApplication::translate("Quassel::secondsToString()", "sec")));

    QStringList returnString;
    for(int i=0; i < timeUnit.size(); i++) {
      int n = timeInSeconds / timeUnit[i].first;
      if(n > 0) {
        returnString += QString("%1 %2").arg(QString::number(n), timeUnit[i].second);
      }
      timeInSeconds = timeInSeconds % timeUnit[i].first;
    }
    return returnString.join(", ");
}
