/***************************************************************************
 *   Copyright (C) 2005/06 by The Quassel Team                             *
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

#ifndef _UTIL_H_
#define _UTIL_H_

#include <QIODevice>
#include <QVariant>
#include <QString>

QString nickFromMask(QString mask);
QString userFromMask(QString mask);
QString hostFromMask(QString mask);

//! Take a string and decode it using the specified text codec, recognizing utf8.
/** This function takes a string and first checks if it is encoded in utf8, in which case it is
 *  decoded appropriately. Otherwise, the specified text codec is used to transform the string.
 *  \param input The input string containing encoded data
 *  \param encoding The text encoding we assume if it's not utf8
 *  \return The decoded string.
 */
QString decodeString(QByteArray input, QString encoding = "ISO-8859-15");

bool isChannelName(QString str);

/**
 *  Writes a QVariant to a device. The data item is prefixed with the resulting blocksize,
 *  so the corresponding function readDataFromDevice() can check if enough data is available
 *  at the device to reread the item.
 */
void writeDataToDevice(QIODevice *, const QVariant &);

/** Reads a data item from a device that has previously been written by writeDataToDevice().
 *  If not enough data bytes are available, the function returns false and the QVariant reference
 *  remains untouched.
 */
bool readDataFromDevice(QIODevice *, quint32 &, QVariant &);





#endif
