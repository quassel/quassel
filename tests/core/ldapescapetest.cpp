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

#include "testglobal.h"

#include "ldapescaper.h"

TEST(LdapEscapeTest, unescaped)
{
    EXPECT_EQ("Babs Jensen",
              LdapEscaper::escapeQuery("Babs Jensen"));
    EXPECT_EQ("Tim Howes",
              LdapEscaper::escapeQuery("Tim Howes"));
    EXPECT_EQ("Babs J\\2a",
              LdapEscaper::escapeQuery("Babs J*"));
}

TEST(LdapEscapeTest, escaped)
{
    EXPECT_EQ("Parens R Us \\28for all your parenthetical needs\\29",
              LdapEscaper::escapeQuery("Parens R Us (for all your parenthetical needs)"));
    EXPECT_EQ("\\2a",
              LdapEscaper::escapeQuery("*"));
    EXPECT_EQ("C:\\5cMyFile",
              LdapEscaper::escapeQuery("C:\\MyFile"));
    EXPECT_EQ("Lu\\c4\\8di\\c4\\87",
              LdapEscaper::escapeQuery("Lu\xc4\x8di\xc4\x87"));
    EXPECT_EQ("\u0004\u0002Hi",
              LdapEscaper::escapeQuery("\x04\x02\x48\x69"));
}
