/***************************************************************************
 *   Copyright (C) 2005-2022 by the Quassel Project                        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include <QDebug>
#include <QDateTime>
#include <QTimeZone>

#include "testglobal.h"
#include "util.h"

TEST(UtilTest, formatDateTimeToOffsetISO)
{
    QDateTime dateTime{{2006, 01, 02}, {15, 04, 05}, QTimeZone{"UTC+01:00"}};

    ASSERT_TRUE(dateTime.isValid());
    ASSERT_FALSE(dateTime.isNull());

    EXPECT_EQ(formatDateTimeToOffsetISO(dateTime), QString("2006-01-02 15:04:05+01:00"));
    EXPECT_EQ(formatDateTimeToOffsetISO(dateTime.toUTC()), QString("2006-01-02 14:04:05Z"));
    EXPECT_EQ(formatDateTimeToOffsetISO(dateTime.toOffsetFromUtc(0)), QString("2006-01-02 14:04:05Z"));
    EXPECT_EQ(formatDateTimeToOffsetISO(dateTime.toOffsetFromUtc(7200)), QString("2006-01-02 16:04:05+02:00"));
    EXPECT_EQ(formatDateTimeToOffsetISO(dateTime.toTimeZone(QTimeZone{"UTC"})), QString("2006-01-02 14:04:05Z"));
}
