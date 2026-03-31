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

#include <QRegularExpression>

#include "clickable.h"
#include "testglobal.h"

TEST(UiSupportRegexTest, clickableListMatchesInternationalizedLinks)
{
    const QString text = QString::fromUtf8("visit http://täst.de/path now");

    const ClickableList clickables = ClickableList::fromString(text);

    ASSERT_EQ(clickables.size(), 1U);
    EXPECT_EQ(clickables[0].type(), Clickable::Url);
    EXPECT_EQ(text.mid(clickables[0].start(), clickables[0].length()), QString::fromUtf8("http://täst.de/path"));
}

TEST(UiSupportRegexTest, tabCompletionSplitKeepsUnicodeWordsIntact)
{
    static const QRegularExpression tabSplitRx(QStringLiteral(R"([^#\w\d_\-\[\]{}|`^.\\])"),
                                               QRegularExpression::UseUnicodePropertiesOption);
    const QStringList parts = QString::fromUtf8("hello Västra").split(tabSplitRx);

    ASSERT_TRUE(tabSplitRx.isValid());
    ASSERT_FALSE(parts.isEmpty());
    EXPECT_EQ(parts.constLast(), QString::fromUtf8("Västra"));
}

TEST(UiSupportRegexTest, nonWordBoundariesDoNotSplitUnicodeWords)
{
    const QString text = QString::fromUtf8("TÜV Västra 狐");
    const qint16 cursor = text.indexOf(QString::fromUtf8("ä"));
    static const QRegularExpression nonWordRx(QStringLiteral("\\W"),
                                              QRegularExpression::UseUnicodePropertiesOption);

    ASSERT_GE(cursor, 0);
    ASSERT_TRUE(nonWordRx.isValid());

    const qint16 start = text.lastIndexOf(nonWordRx, cursor) + 1;
    qint16 end = qMin(text.indexOf(nonWordRx, cursor), text.length());
    if (end < 0)
        end = text.length();

    EXPECT_EQ(text.mid(start, end - start), QString::fromUtf8("Västra"));
}

TEST(UiSupportRegexTest, qssParserAcceptsChatLineConditions)
{
    static const QRegularExpression rx(
            QStringLiteral(R"(ChatLine(?:::(\w+))?(?:#([\w\-]+))?(?:\[([=,\\"\w\s-]+)\])?)"));
    QRegularExpressionMatch match;

    ASSERT_TRUE(rx.isValid());

    match = rx.match(QStringLiteral(R"(ChatLine[label="selected"])"));
    ASSERT_TRUE(match.hasMatch());
    EXPECT_EQ(match.captured(3), QStringLiteral(R"(label="selected")"));
}

TEST(UiSupportRegexTest, qssParserAcceptsListItemConditions)
{
    static const QRegularExpression rx(QStringLiteral(R"((Chat|Nick)ListItem(?:\[([=,\\"\w\s-]+)\])?)"));
    const QRegularExpressionMatch match = rx.match(QStringLiteral(R"(ChatListItem[state="highlighted"])"));

    ASSERT_TRUE(rx.isValid());
    ASSERT_TRUE(match.hasMatch());
    EXPECT_EQ(match.captured(1), QStringLiteral("Chat"));
    EXPECT_EQ(match.captured(2), QStringLiteral(R"(state="highlighted")"));
}
