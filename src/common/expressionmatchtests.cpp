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

#include "expressionmatchtests.h"

#include <QDebug>
#include <QString>
#include <QStringList>
#include <vector>

#include "expressionmatch.h"

void ExpressionMatchTests::runTests()
{
    // Run all tests
    qDebug() << "Running all ExpressionMatch tests";
    runTestsEmptyPattern();
    runTestsMatchPhrase();
    runTestsMatchMultiPhrase();
    runTestsMatchWildcard();
    runTestsMatchMultiWildcard();
    runTestsMatchRegEx();
    runTestsTrimMultiWildcardWhitespace();

    qDebug() << "Passed all ExpressionMatch tests";
}


void ExpressionMatchTests::runTestsEmptyPattern()
{
    qDebug() << "> Testing ExpressionMatch empty pattern handling";

    // Empty pattern
    ExpressionMatch emptyMatch =
            ExpressionMatch("", ExpressionMatch::MatchMode::MatchPhrase, false);

    // Assert empty
    Q_ASSERT(emptyMatch.isEmpty());
    // Assert default match fails (same as setting match empty to false)
    Q_ASSERT(!emptyMatch.match("something"));
    // Assert match empty succeeds
    Q_ASSERT(emptyMatch.match("something", true));
    // Assert empty is valid
    Q_ASSERT(emptyMatch.isValid());

    qDebug() << "* Passed ExpressionMatch empty pattern handling";
}


void ExpressionMatchTests::runTestsMatchPhrase()
{
    qDebug() << "> Testing ExpressionMatch phrase matching";

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
    Q_ASSERT(!simpleMatch.isEmpty());
    Q_ASSERT(simpleMatch.isValid());
    Q_ASSERT(!simpleMatchCS.isEmpty());
    Q_ASSERT(simpleMatchCS.isValid());
    Q_ASSERT(!simpleMatchSpace.isEmpty());
    Q_ASSERT(simpleMatchSpace.isValid());
    Q_ASSERT(!complexMatch.isEmpty());
    Q_ASSERT(complexMatch.isValid());

    // Assert match succeeds
    Q_ASSERT(simpleMatch.match("test"));
    Q_ASSERT(simpleMatch.match("other test;"));
    Q_ASSERT(simpleMatchSpace.match(" space "));
    // Assert partial match fails
    Q_ASSERT(!simpleMatch.match("testing"));
    Q_ASSERT(!simpleMatchSpace.match("space"));
    // Assert unrelated fails
    Q_ASSERT(!simpleMatch.match("not above"));

    // Assert case sensitivity followed
    Q_ASSERT(!simpleMatch.sourceCaseSensitive());
    Q_ASSERT(simpleMatch.match("TeSt"));
    Q_ASSERT(simpleMatchCS.sourceCaseSensitive());
    Q_ASSERT(!simpleMatchCS.match("TeSt"));

    // Assert complex phrases are escaped properly
    Q_ASSERT(complexMatch.match(complexMatchFull));
    Q_ASSERT(!complexMatch.match("norm"));

    qDebug() << "* Passed ExpressionMatch phrase matching";
}


void ExpressionMatchTests::runTestsMatchMultiPhrase()
{
    qDebug() << "> Testing ExpressionMatch multiple phrase matching";

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
    Q_ASSERT(!simpleMatch.isEmpty());
    Q_ASSERT(simpleMatch.isValid());
    Q_ASSERT(!simpleMatchCS.isEmpty());
    Q_ASSERT(simpleMatchCS.isValid());
    Q_ASSERT(!complexMatch.isEmpty());
    Q_ASSERT(complexMatch.isValid());

    // Assert match succeeds
    Q_ASSERT(simpleMatch.match("test"));
    Q_ASSERT(simpleMatch.match("test[suffix]"));
    Q_ASSERT(simpleMatch.match("other test;"));
    Q_ASSERT(simpleMatch.match("Other "));
    Q_ASSERT(simpleMatch.match(".Other !"));
    // Assert partial match fails
    Q_ASSERT(!simpleMatch.match("testing"));
    Q_ASSERT(!simpleMatch.match("Other!"));
    // Assert unrelated fails
    Q_ASSERT(!simpleMatch.match("not above"));

    // Assert case sensitivity followed
    Q_ASSERT(!simpleMatch.sourceCaseSensitive());
    Q_ASSERT(simpleMatch.match("TeSt"));
    Q_ASSERT(simpleMatchCS.sourceCaseSensitive());
    Q_ASSERT(!simpleMatchCS.match("TeSt"));

    // Assert complex phrases are escaped properly
    Q_ASSERT(complexMatch.match(complexMatchFullA));
    Q_ASSERT(complexMatch.match(complexMatchFullB));
    Q_ASSERT(!complexMatch.match("norm"));
    Q_ASSERT(!complexMatch.match("invert"));

    qDebug() << "* Passed ExpressionMatch multiple phrase matching";
}


void ExpressionMatchTests::runTestsMatchWildcard()
{
    qDebug() << "> Testing ExpressionMatch wildcard matching";

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
    Q_ASSERT(!simpleMatch.isEmpty());
    Q_ASSERT(simpleMatch.isValid());
    Q_ASSERT(!simpleMatchCS.isEmpty());
    Q_ASSERT(simpleMatchCS.isValid());
    Q_ASSERT(!simpleMatchEscape.isEmpty());
    Q_ASSERT(simpleMatchEscape.isValid());
    Q_ASSERT(!simpleMatchInvert.isEmpty());
    Q_ASSERT(simpleMatchInvert.isValid());
    Q_ASSERT(!simpleMatchNoInvert.isEmpty());
    Q_ASSERT(simpleMatchNoInvert.isValid());
    Q_ASSERT(!simpleMatchNoInvertSlash.isEmpty());
    Q_ASSERT(simpleMatchNoInvertSlash.isValid());
    Q_ASSERT(!complexMatch.isEmpty());
    Q_ASSERT(complexMatch.isValid());

    // Assert match succeeds
    Q_ASSERT(simpleMatch.match("@test"));
    Q_ASSERT(simpleMatch.match("@testing"));
    Q_ASSERT(simpleMatch.match("!test"));
    Q_ASSERT(simpleMatchEscape.match("?test*"));
    Q_ASSERT(simpleMatchInvert.match("atest"));
    Q_ASSERT(simpleMatchNoInvert.match("!test"));
    Q_ASSERT(simpleMatchNoInvertSlash.match(R"(\!test)"));
    // Assert partial match fails
    Q_ASSERT(!simpleMatch.match("test"));
    // Assert unrelated fails
    Q_ASSERT(!simpleMatch.match("not above"));
    // Assert escaped wildcard fails
    Q_ASSERT(!simpleMatchEscape.match("@testing"));
    Q_ASSERT(!simpleMatchNoInvert.match("test"));
    Q_ASSERT(!simpleMatchNoInvert.match("anything"));
    Q_ASSERT(!simpleMatchNoInvertSlash.match("!test"));
    Q_ASSERT(!simpleMatchNoInvertSlash.match("test"));
    Q_ASSERT(!simpleMatchNoInvertSlash.match("anything"));
    // Assert non-inverted fails
    Q_ASSERT(!simpleMatchInvert.match("testing"));

    // Assert case sensitivity followed
    Q_ASSERT(!simpleMatch.sourceCaseSensitive());
    Q_ASSERT(simpleMatch.match("@TeSt"));
    Q_ASSERT(simpleMatchCS.sourceCaseSensitive());
    Q_ASSERT(!simpleMatchCS.match("@TeSt"));

    // Assert complex match
    Q_ASSERT(complexMatch.match(R"(neverAgonnaBBBgive*you?up\test|yyeah\1\\2\1inval)"));
    // Assert complex not literal match
    Q_ASSERT(!complexMatch.match(R"(never?gonna*give\*you\?up\\test|y\yeah\\1\\\\2\\\1inval)"));
    // Assert complex unrelated not match
    Q_ASSERT(!complexMatch.match("other"));

    qDebug() << "* Passed ExpressionMatch wildcard matching";
}


void ExpressionMatchTests::runTestsMatchMultiWildcard()
{
    qDebug() << "> Testing ExpressionMatch multiple wildcard matching";

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
    Q_ASSERT(!simpleMatch.isEmpty());
    Q_ASSERT(simpleMatch.isValid());
    Q_ASSERT(!simpleMatchCS.isEmpty());
    Q_ASSERT(simpleMatchCS.isValid());
    Q_ASSERT(!simpleMatchEscape.isEmpty());
    Q_ASSERT(simpleMatchEscape.isValid());
    Q_ASSERT(!simpleMatchInvert.isEmpty());
    Q_ASSERT(simpleMatchInvert.isValid());
    Q_ASSERT(!simpleMatchImplicit.isEmpty());
    Q_ASSERT(simpleMatchImplicit.isValid());
    Q_ASSERT(!complexMatch.isEmpty());
    Q_ASSERT(complexMatch.isValid());

    // Assert match succeeds
    Q_ASSERT(simpleMatch.match("@test"));
    Q_ASSERT(simpleMatch.match("@testing"));
    Q_ASSERT(simpleMatch.match("!test"));
    Q_ASSERT(simpleMatch.match("anotherA"));
    Q_ASSERT(simpleMatchEscape.match("?test*;thing*"));
    Q_ASSERT(simpleMatchEscape.match("?test*;AAAAAthing*"));
    Q_ASSERT(simpleMatchInvert.match("test"));
    Q_ASSERT(simpleMatchInvert.match("testing things"));
    // Assert implicit wildcard succeeds
    Q_ASSERT(simpleMatchImplicit.match("AAAAAA"));
    // Assert partial match fails
    Q_ASSERT(!simpleMatch.match("test"));
    Q_ASSERT(!simpleMatch.match("another"));
    Q_ASSERT(!simpleMatch.match("anotherBB"));
    // Assert unrelated fails
    Q_ASSERT(!simpleMatch.match("not above"));
    // Assert escaped wildcard fails
    Q_ASSERT(!simpleMatchEscape.match("@testing"));
    // Assert inverted match fails
    Q_ASSERT(!simpleMatchInvert.match("testing"));
    Q_ASSERT(!simpleMatchImplicit.match("testing"));

    // Assert case sensitivity followed
    Q_ASSERT(!simpleMatch.sourceCaseSensitive());
    Q_ASSERT(simpleMatch.match("@TeSt"));
    Q_ASSERT(simpleMatchCS.sourceCaseSensitive());
    Q_ASSERT(!simpleMatchCS.match("@TeSt"));

    // Assert complex match
    for (auto&& normMatch : complexMatchNormal) {
        // Each normal component should match
        Q_ASSERT(complexMatch.match(normMatch));
    }

    for (auto&& invertMatch : complexMatchInvert) {
        // Each invert component should not match
        Q_ASSERT(!complexMatch.match(invertMatch));
    }

    // Assert complex not literal match
    Q_ASSERT(!complexMatch.match(complexMatchFull));
    // Assert complex unrelated not match
    Q_ASSERT(!complexMatch.match("other"));

    qDebug() << "* Passed ExpressionMatch multiple wildcard matching";
}


void ExpressionMatchTests::runTestsMatchRegEx()
{
    qDebug() << "> Testing ExpressionMatch regex matching";

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
    Q_ASSERT(!simpleMatch.isEmpty());
    Q_ASSERT(simpleMatch.isValid());
    Q_ASSERT(!simpleMatchCS.isEmpty());
    Q_ASSERT(simpleMatchCS.isValid());
    Q_ASSERT(!simpleMatchInvert.isEmpty());
    Q_ASSERT(simpleMatchInvert.isValid());
    Q_ASSERT(!simpleMatchNoInvert.isEmpty());
    Q_ASSERT(simpleMatchNoInvert.isValid());
    Q_ASSERT(!simpleMatchNoInvertSlash.isEmpty());
    Q_ASSERT(simpleMatchNoInvertSlash.isValid());

    // Assert match succeeds
    Q_ASSERT(simpleMatch.match("simpleA*escape-match"));
    Q_ASSERT(simpleMatch.match("simpleA*escape-matchBBBB"));
    Q_ASSERT(simpleMatchInvert.match("not above"));
    Q_ASSERT(simpleMatchNoInvert.match("!simpleA*escape-matchBBBB"));
    Q_ASSERT(simpleMatchNoInvertSlash.match(R"(\!simpleA*escape-matchBBBB)"));
    // Assert partial match fails
    Q_ASSERT(!simpleMatch.match("simpleA*escape-mat"));
    Q_ASSERT(!simpleMatch.match("simple*escape-match"));
    // Assert unrelated fails
    Q_ASSERT(!simpleMatch.match("not above"));
    // Assert escaped wildcard fails
    Q_ASSERT(!simpleMatch.match("simpleABBBBescape-matchBBBB"));
    // Assert inverted fails
    Q_ASSERT(!simpleMatchInvert.match("invertA*escape-match"));
    Q_ASSERT(!simpleMatchInvert.match("invertA*escape-matchBBBB"));
    Q_ASSERT(!simpleMatchNoInvert.match("simpleA*escape-matchBBBB"));
    Q_ASSERT(!simpleMatchNoInvert.match("anything"));
    Q_ASSERT(!simpleMatchNoInvertSlash.match("!simpleA*escape-matchBBBB"));
    Q_ASSERT(!simpleMatchNoInvertSlash.match("anything"));

    // Assert case sensitivity followed
    Q_ASSERT(!simpleMatch.sourceCaseSensitive());
    Q_ASSERT(simpleMatch.match("SiMpLEA*escape-MATCH"));
    Q_ASSERT(simpleMatchCS.sourceCaseSensitive());
    Q_ASSERT(!simpleMatchCS.match("SiMpLEA*escape-MATCH"));

    qDebug() << "* Passed ExpressionMatch regex matching";
}


void ExpressionMatchTests::runTestsTrimMultiWildcardWhitespace()
{
    qDebug() << "> Testing ExpressionMatch multiple wildcard whitespace trimming";

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
        Q_ASSERT(patternPair.size() == 2);
        // Run transformation
        result = ExpressionMatch::trimMultiWildcardWhitespace(patternPair[PATTERN_SOURCE]);
        // Assert that source trims into expected pattern
        Q_ASSERT(result == patternPair[PATTERN_RESULT]);
        // Assert that re-trimming expected pattern gives the same result
        Q_ASSERT(result == ExpressionMatch::trimMultiWildcardWhitespace(result));
    }

    qDebug() << "* Passed ExpressionMatch multiple wildcard whitespace trimming";
}
