/***************************************************************************
 *   Copyright (C) 2005-2019 by the Quassel Project                        *
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

#include <ostream>

#include "testglobal.h"
#include "ircdecoder.h"
#include "irctag.h"

struct IrcMessage
{
    std::map<IrcTagKey, std::string> tags;
    std::string prefix;
    std::string cmd;
    std::vector<std::string> params;

    explicit IrcMessage(std::map<IrcTagKey, std::string> tags, std::string prefix, std::string cmd, std::vector<std::string> params = {})
        : tags(std::move(tags)), prefix(std::move(prefix)), cmd(std::move(cmd)), params(std::move(params)) {}

    explicit IrcMessage(std::initializer_list<std::pair<const IrcTagKey, std::string>> tags, std::string prefix, std::string cmd, std::vector<std::string> params = {})
        : tags(tags), prefix(std::move(prefix)), cmd(std::move(cmd)), params(std::move(params)) {}

    explicit IrcMessage(std::string prefix, std::string cmd, std::vector<std::string> params = {})
        : tags({}), prefix(std::move(prefix)), cmd(std::move(cmd)), params(std::move(params)) {}

    friend bool operator==(const IrcMessage& a, const IrcMessage& b)
    {
        return a.tags == b.tags &&
               a.prefix == b.prefix &&
               a.cmd == b.cmd &&
               a.params == b.params;
    }

    friend std::ostream& operator<<(std::ostream& o, const IrcMessage& m)
    {
        o << "(tags={";
        for (const std::pair<IrcTagKey, std::string> entry: m.tags) {
            o << entry.first << "='" << entry.second << "', ";
        }
        o << "}, prefix=" << m.prefix << ", cmd=" << m.cmd << ", params=[";
        for (const std::string& param : m.params) {
            o << "'" << param << "', ";
        }
        o << "])";
        return o;
    }
};


IrcMessage parse(const std::string& message)
{
    QHash<IrcTagKey, QString> tags;
    QString prefix;
    QString cmd;
    QList<QByteArray> params;

    IrcDecoder::parseMessage([](const QByteArray& data) {
        return QString::fromUtf8(data);
    }, QByteArray::fromStdString(message), tags, prefix, cmd, params);

    std::map<IrcTagKey, std::string> actualTags;
    for (const IrcTagKey& key : tags.keys()) {
        actualTags[key] = tags[key].toStdString();
    }
    std::string actualPrefix = prefix.toStdString();
    std::string actualCmd = cmd.toStdString();
    std::vector<std::string> actualParams;
    for (const QByteArray& param : params) {
        actualParams.push_back(param.toStdString());
    }

    return IrcMessage{actualTags, actualPrefix, actualCmd, actualParams};
}

TEST(IrcDecoderTest, simple)
{
    EXPECT_EQ(parse("foo bar baz asdf"),
              IrcMessage("",
                         "foo",
                         {"bar", "baz", "asdf"}));
}

TEST(IrcDecoderTest, with_source)
{
    EXPECT_EQ(parse(":coolguy foo bar baz asdf"),
              IrcMessage("coolguy",
                         "foo",
                         {"bar", "baz", "asdf"}));
}

TEST(IrcDecoderTest, with_trailing_param)
{
    EXPECT_EQ(parse("foo bar baz :asdf quux"),
              IrcMessage("",
                         "foo",
                         {"bar", "baz", "asdf quux"}));
    EXPECT_EQ(parse("foo bar baz :"),
              IrcMessage("",
                         "foo",
                         {"bar", "baz", ""}));
    EXPECT_EQ(parse("foo bar baz ::asdf"),
              IrcMessage("",
                         "foo",
                         {"bar", "baz", ":asdf"}));
}

TEST(IrcDecoderTest, with_source_and_trailing_param)
{
    EXPECT_EQ(parse(":coolguy foo bar baz :asdf quux"),
              IrcMessage("coolguy",
                         "foo",
                         {"bar", "baz", "asdf quux"}));
    EXPECT_EQ(parse(":coolguy foo bar baz :  asdf quux "),
              IrcMessage("coolguy",
                         "foo",
                         {"bar", "baz", "  asdf quux "}));
    EXPECT_EQ(parse(":coolguy PRIVMSG bar :lol :) "),
              IrcMessage("coolguy",
                         "PRIVMSG",
                         {"bar", "lol :) "}));
    EXPECT_EQ(parse(":coolguy foo bar baz :  "),
              IrcMessage("coolguy",
                         "foo",
                         {"bar", "baz", "  "}));
}

TEST(IrcDecoderTest, with_tags)
{
    EXPECT_EQ(parse("@a=b;c=32;k;rt=ql7 foo"),
              IrcMessage({{IrcTagKey("a"),  "b"},
                          {IrcTagKey("c"),  "32"},
                          {IrcTagKey("k"),  ""},
                          {IrcTagKey("rt"), "ql7"}},
                         "",
                         "foo"));
}

TEST(IrcDecoderTest, with_escaped_tags)
{
    EXPECT_EQ(parse("@a=b\\\\and\\nk;c=72\\s45;d=gh\\:764 foo"),
              IrcMessage({{IrcTagKey("a"), "b\\and\nk"},
                          {IrcTagKey("c"), "72 45"},
                          {IrcTagKey("d"), "gh;764"}},
                         "",
                         "foo"));
}

TEST(IrcDecoderTest, with_tags_and_source)
{
    EXPECT_EQ(parse("@c;h=;a=b :quux ab cd"),
              IrcMessage({{IrcTagKey("c"), ""},
                          {IrcTagKey("h"), ""},
                          {IrcTagKey("a"), "b"}},
                         "quux",
                         "ab",
                         {"cd"}));
}

TEST(IrcDecoderTest, different_forms_of_last_param)
{
    EXPECT_EQ(parse(":src JOIN #chan"),
              IrcMessage("src",
                         "JOIN",
                         {"#chan"}));
    EXPECT_EQ(parse(":src JOIN :#chan"),
              IrcMessage("src",
                         "JOIN",
                         {"#chan"}));
}

TEST(IrcDecoderTest, with_and_without_last_param)
{
    EXPECT_EQ(parse(":src AWAY"),
              IrcMessage("src",
                         "AWAY"));
    EXPECT_EQ(parse(":src AWAY "),
              IrcMessage("src",
                         "AWAY"));
}

TEST(IrcDecoderTest, tab_is_not_considered_SPACE)
{
    EXPECT_EQ(parse(":cool\tguy foo bar baz"),
              IrcMessage("cool\tguy",
                         "foo",
                         {"bar", "baz"}));
}

TEST(IrcDecoderTest, with_weird_control_codes_in_the_source)
{
    EXPECT_EQ(parse(":coolguy!ag@net""\x03""5w""\x03""ork.admin PRIVMSG foo :bar baz"),
              IrcMessage("coolguy!ag@net""\x03""5w""\x03""ork.admin",
                         "PRIVMSG",
                         {"foo", "bar baz"}));
    EXPECT_EQ(parse(":coolguy!~ag@n""\x02""et""\x03""05w""\x0f""ork.admin PRIVMSG foo :bar baz"),
              IrcMessage("coolguy!~ag@n""\x02""et""\x03""05w""\x0f""ork.admin",
                         "PRIVMSG",
                         {"foo", "bar baz"}));
}

TEST(IrcDecoderTest, with_tags_source_and_params)
{
    EXPECT_EQ(parse("@tag1=value1;tag2;vendor1/tag3=value2;vendor2/tag4 :irc.example.com COMMAND param1 param2 :param3 param3"),
              IrcMessage({{IrcTagKey("tag1"),            "value1"},
                          {IrcTagKey("tag2"),            ""},
                          {IrcTagKey("vendor1", "tag3"), "value2"},
                          {IrcTagKey("vendor2", "tag4"), ""}},
                         "irc.example.com",
                         "COMMAND",
                         {"param1", "param2", "param3 param3"}));
    EXPECT_EQ(parse(":irc.example.com COMMAND param1 param2 :param3 param3"),
              IrcMessage("irc.example.com",
                         "COMMAND",
                         {"param1", "param2", "param3 param3"}));
    EXPECT_EQ(parse("@tag1=value1;tag2;vendor1/tag3=value2;vendor2/tag4 COMMAND param1 param2 :param3 param3"),
              IrcMessage({{IrcTagKey("tag1"),            "value1"},
                          {IrcTagKey("tag2"),            ""},
                          {IrcTagKey("vendor1", "tag3"), "value2"},
                          {IrcTagKey("vendor2", "tag4"), ""}},
                         "",
                         "COMMAND",
                         {"param1", "param2", "param3 param3"}));
    EXPECT_EQ(parse("COMMAND"),
              IrcMessage("",
                         "COMMAND"));
    EXPECT_EQ(parse("@foo=\\\\\\\\\\:\\\\s\\s\\r\\n COMMAND"),
              IrcMessage({{IrcTagKey("foo"), "\\\\;\\s \r\n"}},
                         "",
                         "COMMAND"));
}

TEST(IrcDecoderTest, broken_messages_from_unreal)
{
    EXPECT_EQ(parse(":gravel.mozilla.org 432  #momo :Erroneous Nickname: Illegal characters"),
              IrcMessage("gravel.mozilla.org",
                         "432",
                         {"#momo", "Erroneous Nickname: Illegal characters"}));
    EXPECT_EQ(parse(":gravel.mozilla.org MODE #tckk +n "),
              IrcMessage("gravel.mozilla.org",
                         "MODE",
                         {"#tckk", "+n"}));
    EXPECT_EQ(parse(":services.esper.net MODE #foo-bar +o foobar  "),
              IrcMessage("services.esper.net",
                         "MODE",
                         {"#foo-bar", "+o", "foobar"}));
}

TEST(IrcDecoderTest, tag_values)
{
    EXPECT_EQ(parse("@tag1=value\\\\ntest COMMAND"),
              IrcMessage({{IrcTagKey("tag1"), "value\\ntest"}},
                         "",
                         "COMMAND"));
    EXPECT_EQ(parse("@tag1=value\\1 COMMAND"),
              IrcMessage({{IrcTagKey("tag1"), "value1"}},
                         "",
                         "COMMAND"));
    EXPECT_EQ(parse("@tag1=value1\\ COMMAND"),
              IrcMessage({{IrcTagKey("tag1"), "value1"}},
                         "",
                         "COMMAND"));
}
