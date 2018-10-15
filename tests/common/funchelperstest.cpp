/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
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

#include "testglobal.h"

#include <QVariantList>

#include "funchelpers.h"

TEST(FuncHelpersTest, invokeWithArgsList)
{
    int intVal{};
    QString stringVal{};

    auto callable = [&intVal, &stringVal](int i, const QString& s)
    {
        intVal = i;
        stringVal = s;
    };

    // Good case
    {
        QVariantList argsList{42, "Hello World"};
        ASSERT_TRUE(invokeWithArgsList(callable, argsList));
        EXPECT_EQ(42, intVal);
        EXPECT_EQ("Hello World", stringVal);
    }

    // Too many arguments
    {
        QVariantList argsList{23, "Hi Universe", 2.3};
        ASSERT_FALSE(invokeWithArgsList(callable, argsList));
        // Values shouldn't have changed
        EXPECT_EQ(42, intVal);
        EXPECT_EQ("Hello World", stringVal);
    }

    // Too few arguments
    {
        QVariantList argsList{23};
        ASSERT_FALSE(invokeWithArgsList(callable, argsList));
        // Values shouldn't have changed
        EXPECT_EQ(42, intVal);
        EXPECT_EQ("Hello World", stringVal);
    }

    // Cannot convert argument type
    {
        // Ensure type cannot be converted
        QVariantList wrong{"Foo", "Bar"};
        QVariant v{wrong};
        ASSERT_FALSE(v.canConvert<QString>());

        QVariantList argsList{23, wrong};
        ASSERT_FALSE(invokeWithArgsList(callable, argsList));
        // Values shouldn't have changed
        EXPECT_EQ(42, intVal);
        EXPECT_EQ("Hello World", stringVal);
    }
}
