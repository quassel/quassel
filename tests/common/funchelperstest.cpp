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

TEST(FuncHelpersTest, invokeLambdaWithArgsList)
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
        auto ret = invokeWithArgsList(callable, {42, "Hello World"});
        ASSERT_TRUE(ret);
        EXPECT_FALSE(ret->isValid());  // Callable returns void, so the returned QVariant should be invalid
        EXPECT_EQ(42, intVal);
        EXPECT_EQ("Hello World", stringVal);
    }

    // Too many arguments
    {
        ASSERT_FALSE(invokeWithArgsList(callable, {23, "Hi Universe", 2.3}));
        // Values shouldn't have changed
        EXPECT_EQ(42, intVal);
        EXPECT_EQ("Hello World", stringVal);
    }

    // Too few arguments
    {
        ASSERT_FALSE(invokeWithArgsList(callable, {23}));
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

        ASSERT_FALSE(invokeWithArgsList(callable, {23, wrong}));
        // Values shouldn't have changed
        EXPECT_EQ(42, intVal);
        EXPECT_EQ("Hello World", stringVal);
    }
}

TEST(FuncHelpersTest, invokeLambdaWithArgsListAndReturnValue)
{
    int intVal{};
    QString stringVal{};

    auto callable = [&intVal, &stringVal](int i, const QString& s)
    {
        intVal = i;
        stringVal = s;
        return -i;
    };

    // Good case
    {
        auto ret = invokeWithArgsList(callable, {42, "Hello World"});
        ASSERT_TRUE(ret);
        ASSERT_TRUE(ret->isValid());
        EXPECT_EQ(-42, *ret);
        EXPECT_EQ(42, intVal);
        EXPECT_EQ("Hello World", stringVal);
    }

    // Failed invocation
    {
        ASSERT_FALSE(invokeWithArgsList(callable, {23}));
    }
}

class Object
{
public:
    void voidFunc(int i, const QString& s)
    {
        intVal = i;
        stringVal = s;
    }

    int intFunc(int i)
    {
        return -i;
    }

    int intVal{};
    QString stringVal{};
};

TEST(FuncHelpersTest, invokeMemberFunction)
{
    Object object;

    // Good case
    {
        auto ret = invokeWithArgsList(&object, &Object::voidFunc, {42, "Hello World"});
        ASSERT_TRUE(ret);
        EXPECT_FALSE(ret->isValid());  // Callable returns void, so the returned QVariant should be invalid
        EXPECT_EQ(42, object.intVal);
        EXPECT_EQ("Hello World", object.stringVal);
    }

    // Good case with return value
    {
        auto ret = invokeWithArgsList(&object, &Object::intFunc, {42});
        ASSERT_TRUE(ret);
        EXPECT_EQ(-42, *ret);
    }

    // Too few arguments
    {
        auto ret = invokeWithArgsList(&object, &Object::voidFunc, {23});
        ASSERT_FALSE(ret);
        EXPECT_EQ(42, object.intVal);
    }
}
