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

#include <vector>

#include <QString>
#include <QStringList>

#include "testglobal.h"
#include "expressionmatch.h"

TEST(ExpressionMatchTest, emptyPattern)
{
    // Empty pattern
    ExpressionMatch emptyMatch =
            ExpressionMatch("", ExpressionMatch::MatchMode::MatchPhrase, false);

    // Assert empty is valid
    ASSERT_TRUE(emptyMatch.isValid());
    // Assert empty
    EXPECT_TRUE(emptyMatch.isEmpty());
    // Assert default match fails (same as setting match empty to false)
    EXPECT_FALSE(emptyMatch.match("something"));
    // Assert match empty succeeds
    EXPECT_TRUE(emptyMatch.match("something", true));
}

TEST(ExpressionMatchTest, matchPhrase)
{
    // Simple phrase, case-insensitive
    ExpressionMatch simpleMatch =
            ExpressionMatch("test", ExpressionMatch::MatchMode::MatchPhrase, false);
    // Simple phrase, case-sensitive
    ExpressionMatch simpleMatchCS =
            ExpressionMatch("test", ExpressionMatch::MatchMode::MatchPhrase, true);
    // Phrase with space, case-insensitive
    ExpressionMatch simpleMatchSpace =
            ExpressionMatch(" space ", ExpressionMatch::MatchMode::MatchPhrase, true);
    // Complex phrase
    QString complexMatchFull(R"(^(?:norm|norm\-space|\!norm\-escaped|\\\!slash\-invert|\\\\double"
                              "|escape\;sep|slash\-end\-split\\|quad\\\\\!noninvert|newline\-split"
                              "|newline\-split\-slash\\|slash\-at\-end\\)$)");
    ExpressionMatch complexMatch =
            ExpressionMatch(complexMatchFull, ExpressionMatch::MatchMode::MatchPhrase, false);

    // Assert valid and not empty
    ASSERT_TRUE(simpleMatch.isValid());
    EXPECT_FALSE(simpleMatch.isEmpty());
    ASSERT_TRUE(simpleMatchCS.isValid());
    EXPECT_FALSE(simpleMatchCS.isEmpty());
    ASSERT_TRUE(simpleMatchSpace.isValid());
    EXPECT_FALSE(simpleMatchSpace.isEmpty());
    ASSERT_TRUE(complexMatch.isValid());
    EXPECT_FALSE(complexMatch.isEmpty());

    // Assert match succeeds
    EXPECT_TRUE(simpleMatch.match("test"));
    EXPECT_TRUE(simpleMatch.match("other test;"));
    EXPECT_TRUE(simpleMatchSpace.match(" space "));
    // Assert partial match fails
    EXPECT_FALSE(simpleMatch.match("testing"));
    EXPECT_FALSE(simpleMatchSpace.match("space"));
    // Assert unrelated fails
    EXPECT_FALSE(simpleMatch.match("not above"));

    // Assert case sensitivity followed
    EXPECT_FALSE(simpleMatch.sourceCaseSensitive());
    EXPECT_TRUE(simpleMatch.match("TeSt"));
    EXPECT_TRUE(simpleMatchCS.sourceCaseSensitive());
    EXPECT_FALSE(simpleMatchCS.match("TeSt"));

    // Assert complex phrases are escaped properly
    EXPECT_TRUE(complexMatch.match(complexMatchFull));
    EXPECT_FALSE(complexMatch.match("norm"));
}


TEST(ExpressionMatchTest, matchMultiPhrase)
{
    // Simple phrases, case-insensitive
    ExpressionMatch simpleMatch =
            ExpressionMatch("test\nOther ", ExpressionMatch::MatchMode::MatchMultiPhrase, false);
    // Simple phrases, case-sensitive
    ExpressionMatch simpleMatchCS =
            ExpressionMatch("test\nOther ", ExpressionMatch::MatchMode::MatchMultiPhrase, true);
    // Complex phrases
    QString complexMatchFullA(R"(^(?:norm|norm\-space|\!norm\-escaped|\\\!slash\-invert|\\\\double)"
                              R"(|escape\;sep|slash\-end\-split\\|quad\\\\\!noninvert)"
                              R"(|newline\-split|newline\-split\-slash\\|slash\-at\-end\\)$)");
    QString complexMatchFullB(R"(^(?:invert|invert\-space)$)$)");
    ExpressionMatch complexMatch =
            ExpressionMatch(complexMatchFullA + "\n" + complexMatchFullB,
                            ExpressionMatch::MatchMode::MatchMultiPhrase, false);

    // Assert valid and not empty
    ASSERT_TRUE(simpleMatch.isValid());
    EXPECT_FALSE(simpleMatch.isEmpty());
    ASSERT_TRUE(simpleMatchCS.isValid());
    EXPECT_FALSE(simpleMatchCS.isEmpty());
    ASSERT_TRUE(complexMatch.isValid());
    EXPECT_FALSE(complexMatch.isEmpty());

    // Assert match succeeds
    EXPECT_TRUE(simpleMatch.match("test"));
    EXPECT_TRUE(simpleMatch.match("test[suffix]"));
    EXPECT_TRUE(simpleMatch.match("other test;"));
    EXPECT_TRUE(simpleMatch.match("Other "));
    EXPECT_TRUE(simpleMatch.match(".Other !"));
    // Assert partial match fails
    EXPECT_FALSE(simpleMatch.match("testing"));
    EXPECT_FALSE(simpleMatch.match("Other!"));
    // Assert unrelated fails
    EXPECT_FALSE(simpleMatch.match("not above"));

    // Assert case sensitivity followed
    EXPECT_FALSE(simpleMatch.sourceCaseSensitive());
    EXPECT_TRUE(simpleMatch.match("TeSt"));
    EXPECT_TRUE(simpleMatchCS.sourceCaseSensitive());
    EXPECT_FALSE(simpleMatchCS.match("TeSt"));

    // Assert complex phrases are escaped properly
    EXPECT_TRUE(complexMatch.match(complexMatchFullA));
    EXPECT_TRUE(complexMatch.match(complexMatchFullB));
    EXPECT_FALSE(complexMatch.match("norm"));
    EXPECT_FALSE(complexMatch.match("invert"));
}


TEST(ExpressionMatchTest, matchWildcard)
{
    // Simple wildcard, case-insensitive
    ExpressionMatch simpleMatch =
            ExpressionMatch("?test*", ExpressionMatch::MatchMode::MatchWildcard, false);
    // Simple wildcard, case-sensitive
    ExpressionMatch simpleMatchCS =
            ExpressionMatch("?test*", ExpressionMatch::MatchMode::MatchWildcard, true);
    // Escaped wildcard, case-insensitive
    ExpressionMatch simpleMatchEscape =
            ExpressionMatch(R"(\?test\*)", ExpressionMatch::MatchMode::MatchWildcard, false);
    // Inverted wildcard, case-insensitive
    ExpressionMatch simpleMatchInvert =
            ExpressionMatch("!test*", ExpressionMatch::MatchMode::MatchWildcard, false);
    // Not inverted wildcard, case-insensitive
    ExpressionMatch simpleMatchNoInvert =
            ExpressionMatch(R"(\!test*)", ExpressionMatch::MatchMode::MatchWildcard, false);
    // Not inverted wildcard literal slash, case-insensitive
    ExpressionMatch simpleMatchNoInvertSlash =
            ExpressionMatch(R"(\\!test*)", ExpressionMatch::MatchMode::MatchWildcard, false);
    // Complex wildcard
    ExpressionMatch complexMatch =
            ExpressionMatch(R"(never?gonna*give\*you\?up\\test|y\yeah\\1\\\\2\\\1inval)",
                            ExpressionMatch::MatchMode::MatchWildcard, false);

    // Assert valid and not empty
    ASSERT_TRUE(simpleMatch.isValid());
    EXPECT_FALSE(simpleMatch.isEmpty());
    ASSERT_TRUE(simpleMatchCS.isValid());
    EXPECT_FALSE(simpleMatchCS.isEmpty());
    ASSERT_TRUE(simpleMatchEscape.isValid());
    EXPECT_FALSE(simpleMatchEscape.isEmpty());
    ASSERT_TRUE(simpleMatchInvert.isValid());
    EXPECT_FALSE(simpleMatchInvert.isEmpty());
    ASSERT_TRUE(simpleMatchNoInvert.isValid());
    EXPECT_FALSE(simpleMatchNoInvert.isEmpty());
    ASSERT_TRUE(simpleMatchNoInvertSlash.isValid());
    EXPECT_FALSE(simpleMatchNoInvertSlash.isEmpty());
    ASSERT_TRUE(complexMatch.isValid());
    EXPECT_FALSE(complexMatch.isEmpty());

    // Assert match succeeds
    EXPECT_TRUE(simpleMatch.match("@test"));
    EXPECT_TRUE(simpleMatch.match("@testing"));
    EXPECT_TRUE(simpleMatch.match("!test"));
    EXPECT_TRUE(simpleMatchEscape.match("?test*"));
    EXPECT_TRUE(simpleMatchInvert.match("atest"));
    EXPECT_TRUE(simpleMatchNoInvert.match("!test"));
    EXPECT_TRUE(simpleMatchNoInvertSlash.match(R"(\!test)"));
    // Assert partial match fails
    EXPECT_FALSE(simpleMatch.match("test"));
    // Assert unrelated fails
    EXPECT_FALSE(simpleMatch.match("not above"));
    // Assert escaped wildcard fails
    EXPECT_FALSE(simpleMatchEscape.match("@testing"));
    EXPECT_FALSE(simpleMatchNoInvert.match("test"));
    EXPECT_FALSE(simpleMatchNoInvert.match("anything"));
    EXPECT_FALSE(simpleMatchNoInvertSlash.match("!test"));
    EXPECT_FALSE(simpleMatchNoInvertSlash.match("test"));
    EXPECT_FALSE(simpleMatchNoInvertSlash.match("anything"));
    // Assert non-inverted fails
    EXPECT_FALSE(simpleMatchInvert.match("testing"));

    // Assert case sensitivity followed
    EXPECT_FALSE(simpleMatch.sourceCaseSensitive());
    EXPECT_TRUE(simpleMatch.match("@TeSt"));
    EXPECT_TRUE(simpleMatchCS.sourceCaseSensitive());
    EXPECT_FALSE(simpleMatchCS.match("@TeSt"));

    // Assert complex match
    EXPECT_TRUE(complexMatch.match(R"(neverAgonnaBBBgive*you?up\test|yyeah\1\\2\1inval)"));
    // Assert complex not literal match
    EXPECT_FALSE(complexMatch.match(R"(never?gonna*give\*you\?up\\test|y\yeah\\1\\\\2\\\1inval)"));
    // Assert complex unrelated not match
    EXPECT_FALSE(complexMatch.match("other"));
}


TEST(ExpressionMatchTest, matchMultiWildcard)
{
    // Simple wildcards, case-insensitive
    ExpressionMatch simpleMatch =
            ExpressionMatch("?test*;another?",
                            ExpressionMatch::MatchMode::MatchMultiWildcard, false);
    // Simple wildcards, case-sensitive
    ExpressionMatch simpleMatchCS =
            ExpressionMatch("?test*;another?",
                            ExpressionMatch::MatchMode::MatchMultiWildcard, true);
    // Escaped wildcards, case-insensitive
    ExpressionMatch simpleMatchEscape =
            ExpressionMatch(R"(\?test\*\;*thing\*)",
                            ExpressionMatch::MatchMode::MatchMultiWildcard, false);
    // Inverted wildcards, case-insensitive
    ExpressionMatch simpleMatchInvert =
            ExpressionMatch(R"(test*;!testing)",
                            ExpressionMatch::MatchMode::MatchMultiWildcard, false);
    // Implicit wildcards, case-insensitive
    ExpressionMatch simpleMatchImplicit =
            ExpressionMatch(R"(!testing*)",
                            ExpressionMatch::MatchMode::MatchMultiWildcard, false);
    // Complex wildcard
    QString complexMatchFull(R"(norm;!invert; norm-space ; !invert-space ;;!;\!norm-escaped;)"
                             R"(\\!slash-invert;\\\\double; escape\;sep;slash-end-split\\;)"
                             R"(quad\\\\!noninvert;newline-split)""\n"
                             R"(newline-split-slash\\)""\n"
                             R"(slash-at-end\\)");
    // Match normal components
    QStringList complexMatchNormal = {
        R"(norm)",
        R"(norm-space)",
        R"(!norm-escaped)",
        R"(\!slash-invert)",
        R"(\\double)",
        R"(escape;sep)",
        R"(slash-end-split\)",
        R"(quad\\!noninvert)",
        R"(newline-split)",
        R"(newline-split-slash\)",
        R"(slash-at-end\)"
    };
    // Match negating components
    QStringList complexMatchInvert = {
        R"(invert)",
        R"(invert-space)"
    };
    ExpressionMatch complexMatch =
            ExpressionMatch(complexMatchFull, ExpressionMatch::MatchMode::MatchMultiWildcard,
                            false);

    // Assert valid and not empty
    ASSERT_TRUE(simpleMatch.isValid());
    EXPECT_FALSE(simpleMatch.isEmpty());
    ASSERT_TRUE(simpleMatchCS.isValid());
    EXPECT_FALSE(simpleMatchCS.isEmpty());
    ASSERT_TRUE(simpleMatchEscape.isValid());
    EXPECT_FALSE(simpleMatchEscape.isEmpty());
    ASSERT_TRUE(simpleMatchInvert.isValid());
    EXPECT_FALSE(simpleMatchInvert.isEmpty());
    ASSERT_TRUE(simpleMatchImplicit.isValid());
    EXPECT_FALSE(simpleMatchImplicit.isEmpty());
    ASSERT_TRUE(complexMatch.isValid());
    EXPECT_FALSE(complexMatch.isEmpty());

    // Assert match succeeds
    EXPECT_TRUE(simpleMatch.match("@test"));
    EXPECT_TRUE(simpleMatch.match("@testing"));
    EXPECT_TRUE(simpleMatch.match("!test"));
    EXPECT_TRUE(simpleMatch.match("anotherA"));
    EXPECT_TRUE(simpleMatchEscape.match("?test*;thing*"));
    EXPECT_TRUE(simpleMatchEscape.match("?test*;AAAAAthing*"));
    EXPECT_TRUE(simpleMatchInvert.match("test"));
    EXPECT_TRUE(simpleMatchInvert.match("testing things"));
    // Assert implicit wildcard succeeds
    EXPECT_TRUE(simpleMatchImplicit.match("AAAAAA"));
    // Assert partial match fails
    EXPECT_FALSE(simpleMatch.match("test"));
    EXPECT_FALSE(simpleMatch.match("another"));
    EXPECT_FALSE(simpleMatch.match("anotherBB"));
    // Assert unrelated fails
    EXPECT_FALSE(simpleMatch.match("not above"));
    // Assert escaped wildcard fails
    EXPECT_FALSE(simpleMatchEscape.match("@testing"));
    // Assert inverted match fails
    EXPECT_FALSE(simpleMatchInvert.match("testing"));
    EXPECT_FALSE(simpleMatchImplicit.match("testing"));

    // Assert case sensitivity followed
    EXPECT_FALSE(simpleMatch.sourceCaseSensitive());
    EXPECT_TRUE(simpleMatch.match("@TeSt"));
    EXPECT_TRUE(simpleMatchCS.sourceCaseSensitive());
    EXPECT_FALSE(simpleMatchCS.match("@TeSt"));

    // Assert complex match
    for (auto&& normMatch : complexMatchNormal) {
        // Each normal component should match
        EXPECT_TRUE(complexMatch.match(normMatch));
    }

    for (auto&& invertMatch : complexMatchInvert) {
        // Each invert component should not match
        EXPECT_FALSE(complexMatch.match(invertMatch));
    }

    // Assert complex not literal match
    EXPECT_FALSE(complexMatch.match(complexMatchFull));
    // Assert complex unrelated not match
    EXPECT_FALSE(complexMatch.match("other"));
}


TEST(ExpressionMatchTest, matchRegEx)
{
    // Simple regex, case-insensitive
    ExpressionMatch simpleMatch =
            ExpressionMatch(R"(simple.\*escape-match.*)",
                            ExpressionMatch::MatchMode::MatchRegEx, false);
    // Simple regex, case-sensitive
    ExpressionMatch simpleMatchCS =
            ExpressionMatch(R"(simple.\*escape-match.*)",
                            ExpressionMatch::MatchMode::MatchRegEx, true);
    // Inverted regex, case-insensitive
    ExpressionMatch simpleMatchInvert =
            ExpressionMatch(R"(!invert.\*escape-match.*)",
                            ExpressionMatch::MatchMode::MatchRegEx, false);
    // Non-inverted regex, case-insensitive
    ExpressionMatch simpleMatchNoInvert =
            ExpressionMatch(R"(\!simple.\*escape-match.*)",
                            ExpressionMatch::MatchMode::MatchRegEx, false);
    // Non-inverted regex literal slash, case-insensitive
    ExpressionMatch simpleMatchNoInvertSlash =
            ExpressionMatch(R"(\\!simple.\*escape-match.*)",
                            ExpressionMatch::MatchMode::MatchRegEx, false);

    // Assert valid and not empty
    ASSERT_TRUE(simpleMatch.isValid());
    EXPECT_FALSE(simpleMatch.isEmpty());
    ASSERT_TRUE(simpleMatchCS.isValid());
    EXPECT_FALSE(simpleMatchCS.isEmpty());
    ASSERT_TRUE(simpleMatchInvert.isValid());
    EXPECT_FALSE(simpleMatchInvert.isEmpty());
    ASSERT_TRUE(simpleMatchNoInvert.isValid());
    EXPECT_FALSE(simpleMatchNoInvert.isEmpty());
    ASSERT_TRUE(simpleMatchNoInvertSlash.isValid());
    EXPECT_FALSE(simpleMatchNoInvertSlash.isEmpty());

    // Assert match succeeds
    EXPECT_TRUE(simpleMatch.match("simpleA*escape-match"));
    EXPECT_TRUE(simpleMatch.match("simpleA*escape-matchBBBB"));
    EXPECT_TRUE(simpleMatchInvert.match("not above"));
    EXPECT_TRUE(simpleMatchNoInvert.match("!simpleA*escape-matchBBBB"));
    EXPECT_TRUE(simpleMatchNoInvertSlash.match(R"(\!simpleA*escape-matchBBBB)"));
    // Assert partial match fails
    EXPECT_FALSE(simpleMatch.match("simpleA*escape-mat"));
    EXPECT_FALSE(simpleMatch.match("simple*escape-match"));
    // Assert unrelated fails
    EXPECT_FALSE(simpleMatch.match("not above"));
    // Assert escaped wildcard fails
    EXPECT_FALSE(simpleMatch.match("simpleABBBBescape-matchBBBB"));
    // Assert inverted fails
    EXPECT_FALSE(simpleMatchInvert.match("invertA*escape-match"));
    EXPECT_FALSE(simpleMatchInvert.match("invertA*escape-matchBBBB"));
    EXPECT_FALSE(simpleMatchNoInvert.match("simpleA*escape-matchBBBB"));
    EXPECT_FALSE(simpleMatchNoInvert.match("anything"));
    EXPECT_FALSE(simpleMatchNoInvertSlash.match("!simpleA*escape-matchBBBB"));
    EXPECT_FALSE(simpleMatchNoInvertSlash.match("anything"));

    // Assert case sensitivity followed
    EXPECT_FALSE(simpleMatch.sourceCaseSensitive());
    EXPECT_TRUE(simpleMatch.match("SiMpLEA*escape-MATCH"));
    EXPECT_TRUE(simpleMatchCS.sourceCaseSensitive());
    EXPECT_FALSE(simpleMatchCS.match("SiMpLEA*escape-MATCH"));
}


TEST(ExpressionMatchTest, trimMultiWildcardWhitespace)
{
    // Patterns
    static constexpr uint PATTERN_SOURCE = 0;
    static constexpr uint PATTERN_RESULT = 1;
    std::vector<std::vector<QString>> patterns = {
        // Literal
        {"literal",
         "literal"},
        // Simple semicolon cleanup
        {"simple1  ;simple2; simple3 ",
         "simple1; simple2; simple3"},
        // Simple newline cleanup
        {"simple1  \nsimple2\n simple3 ",
         "simple1\nsimple2\nsimple3"},
        // Complex cleanup
        {R"(norm; norm-space ; newline-space )""\n"
         R"( ;escape \; sep ; slash-end-split\\; quad\\\\norm; newline-split-slash\\)""\n"
         R"(slash-at-end\\)",
         R"(norm; norm-space; newline-space)""\n"
         R"(escape \; sep; slash-end-split\\; quad\\\\norm; newline-split-slash\\)""\n"
         R"(slash-at-end\\)"}
    };

    // Check every source string...
    QString result;
    for (auto&& patternPair : patterns) {
        // Make sure data is valid
        ASSERT_TRUE(patternPair.size() == 2);
        // Run transformation
        result = ExpressionMatch::trimMultiWildcardWhitespace(patternPair[PATTERN_SOURCE]);
        // Assert that source trims into expected pattern
        EXPECT_EQ(patternPair[PATTERN_RESULT], result);
        // Assert that re-trimming expected pattern gives the same result
        EXPECT_EQ(ExpressionMatch::trimMultiWildcardWhitespace(result), result);
    }
}


TEST(ExpressionMatchTest, testInvalidRegEx)
{
    // Invalid regular expression pattern
    ExpressionMatch invalidRegExMatch =
            ExpressionMatch("*network", ExpressionMatch::MatchMode::MatchRegEx, false);

    // Assert not valid
    ASSERT_FALSE(invalidRegExMatch.isValid());
    // Assert not empty
    EXPECT_FALSE(invalidRegExMatch.isEmpty());
    // Assert default match fails
    EXPECT_FALSE(invalidRegExMatch.match(""));
    // Assert wildcard match fails
    EXPECT_FALSE(invalidRegExMatch.match("network"));
    // Assert literal match fails
    EXPECT_FALSE(invalidRegExMatch.match("*network"));
}
