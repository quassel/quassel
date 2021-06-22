/***************************************************************************
 *   Copyright (C) 2005-2020 by the Quassel Project                        *
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

#include "irc/ircencoder.h"

#include <ostream>

#include "testglobal.h"

#include "irc/irctagkey.h"

std::string write(const IrcMessage& message)
{
    return IrcEncoder::writeMessage([&](const QString& data) { return data.toUtf8(); }, message).toStdString();
}

TEST(IrcEncoderTest, simple_test_with_verb_and_params)
{
    EXPECT_STRCASEEQ("foo bar baz asdf",
                     write(IrcMessage({}, {}, "foo", {"bar", "baz", "asdf"})).data());
}

TEST(IrcEncoderTest, simple_test_with_source_and_no_params)
{
    EXPECT_STRCASEEQ(":src AWAY",
                     write(IrcMessage({}, "src", "AWAY", {})).data());
}

TEST(IrcEncoderTest, simple_test_with_source_and_empty_trailing_param)
{
    EXPECT_STRCASEEQ(":src AWAY :",
                     write(IrcMessage({}, "src", "AWAY", {""})).data());
}

TEST(IrcEncoderTest, simple_test_with_source)
{
    EXPECT_STRCASEEQ(":coolguy foo bar baz asdf",
                     write(IrcMessage({}, "coolguy", "foo", {"bar", "baz", "asdf"})).data());
}

TEST(IrcEncoderTest, simple_test_with_trailing_param)
{
    EXPECT_STRCASEEQ("foo bar baz :asdf quux",
                     write(IrcMessage({}, "", "foo", {"bar", "baz", "asdf quux"})).data());
}

TEST(IrcEncoderTest, simple_test_with_empty_trailing_param)
{
    EXPECT_STRCASEEQ("foo bar baz :",
                     write(IrcMessage({}, "", "foo", {"bar", "baz", ""})).data());
}

TEST(IrcEncoderTest, simple_test_with_trailing_param_containing_colon)
{
    EXPECT_STRCASEEQ("foo bar baz ::asdf",
                     write(IrcMessage({}, "", "foo", {"bar", "baz", ":asdf"})).data());
}

TEST(IrcEncoderTest, test_with_source_and_trailing_param)
{
    EXPECT_STRCASEEQ(":coolguy foo bar baz :asdf quux",
                     write(IrcMessage({}, "coolguy", "foo", {"bar", "baz", "asdf quux"})).data());
}

TEST(IrcEncoderTest, test_with_trailing_containing_beginning_end_whitespace)
{
    EXPECT_STRCASEEQ(":coolguy foo bar baz :  asdf quux ",
                     write(IrcMessage({}, "coolguy", "foo", {"bar", "baz", "  asdf quux "})).data());
}

TEST(IrcEncoderTest, test_with_trailing_containing_what_looks_like_another_trailing_param)
{
    EXPECT_STRCASEEQ(":coolguy PRIVMSG bar :lol :) ",
                     write(IrcMessage({}, "coolguy", "PRIVMSG", {"bar", "lol :) "})).data());
}

TEST(IrcEncoderTest, simple_test_with_source_and_empty_trailing)
{
    EXPECT_STRCASEEQ(":coolguy foo bar baz :",
                     write(IrcMessage({}, "coolguy", "foo", {"bar", "baz", ""})).data());
}

TEST(IrcEncoderTest, trailing_contains_only_spaces)
{
    EXPECT_STRCASEEQ(":coolguy foo bar baz :  ",
                     write(IrcMessage({}, "coolguy", "foo", {"bar", "baz", "  "})).data());
}

TEST(IrcEncoderTest, param_containing_tab_tab_is_not_considered_SPACE_for_message_splitting)
{
    EXPECT_STRCASEEQ(":coolguy foo b\tar baz",
                     write(IrcMessage({}, "coolguy", "foo", {"b\tar", "baz"})).data());
}

TEST(IrcEncoderTest, tags_with_no_value_and_space_filled_trailing)
{
    EXPECT_STRCASEEQ("@asd :coolguy foo bar baz :  ",
                     write(IrcMessage({{IrcTagKey("asd"), ""}}, "coolguy", "foo", {"bar", "baz", "  "})).data());
}

TEST(IrcEncoderTest, tags_with_invalid_vendor)
{
    EXPECT_STRCASEEQ("@a=b foo",
                     write(IrcMessage({{IrcTagKey("a"), "b"}}, {}, "foo", {})).data());
    EXPECT_STRCASEEQ("@example.com/a=b foo",
                     write(IrcMessage({{IrcTagKey("example.com", "a"), "b"}}, {}, "foo", {})).data());
    EXPECT_STRCASEEQ("@example.com/subfolder/to/a=b foo",
                     write(IrcMessage({{IrcTagKey("example.com/subfolder/to", "a"), "b"}}, {}, "foo", {})).data());
    EXPECT_STRCASEEQ("@v\\/e\\/n\\/d\\/o\\/r/tag=b foo",
                     write(IrcMessage({{IrcTagKey("v\\/e\\/n\\/d\\/o\\/r", "tag"), "b"}}, {}, "foo", {})).data());
}

TEST(IrcEncoderTest, tags_with_escaped_values)
{
    std::vector<std::string> expected{
        R"(@d=gh\:764;a=b\\and\nk foo)",
        R"(@a=b\\and\nk;d=gh\:764 foo)",
    };
    EXPECT_THAT(expected,
                testing::Contains(testing::StrCaseEq(write(IrcMessage({{IrcTagKey("a"), "b\\and\nk"}, {IrcTagKey("d"), "gh;764"}}, {}, "foo", {})))));
}

TEST(IrcEncoderTest, tags_with_escaped_values_and_params)
{
    std::vector<std::string> expected{
        R"(@d=gh\:764;a=b\\and\nk foo par1 par2)",
        R"(@a=b\\and\nk;d=gh\:764 foo par1 par2)",
    };
    EXPECT_THAT(expected,
                testing::Contains(testing::StrCaseEq(
                    write(IrcMessage({{IrcTagKey("a"), "b\\and\nk"}, {IrcTagKey("d"), "gh;764"}}, {}, "foo", {"par1", "par2"})))));
}

TEST(IrcEncoderTest, tags_with_long_strange_values)
{
    EXPECT_STRCASEEQ(R"(@foo=\\\\\:\\s\s\r\n COMMAND)",
                     write(IrcMessage({{IrcTagKey("foo"), "\\\\;\\s \r\n"}}, {}, "COMMAND", {})).data());
}
