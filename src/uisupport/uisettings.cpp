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

#include "uisettings.h"

UiSettings::UiSettings(const QString &group)
  : ClientSettings(group)
{
}

/**************************************************************************/

UiStyleSettings::UiStyleSettings(const QString &group)
  : ClientSettings(group)
{
}

void UiStyleSettings::setCustomFormat(UiStyle::FormatType ftype, QTextCharFormat format) {
  setLocalValue(QString("Format/%1").arg(ftype), format);
}

QTextCharFormat UiStyleSettings::customFormat(UiStyle::FormatType ftype) {
  return localValue(QString("Format/%1").arg(ftype), QTextFormat()).value<QTextFormat>().toCharFormat();
}

void UiStyleSettings::removeCustomFormat(UiStyle::FormatType ftype) {
  removeLocalKey(QString("Format/%1").arg(ftype));
}

QList<UiStyle::FormatType> UiStyleSettings::availableFormats() {
  QList<UiStyle::FormatType> formats;
  QStringList list = localChildKeys("Format");
  foreach(QString type, list) {
    formats << (UiStyle::FormatType)type.toInt();
  }
  return formats;
}
