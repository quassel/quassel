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

#pragma once

#include <QString>
#include <QStringList>

/**
 * Expression matcher test runner (temporary measure until switching to a real test framework)
 */
class ExpressionMatchTests
{

public:
    /**
     * Runs the ExpressionMatch tests
     *
     * Q_ASSERT will deliberately halt execution if an issue is found.  This should not be called
     * outside of a development environment.
     *
     * Recommended development usage until a testing framework exists:
     * 1.  Include "expressionmatchtests.h" in quassel.cpp
     * 2.  Call this function near the end of Quassel::init()
     */
    static void runTests();

private:
    // Disallow creating an instance of this static object
    /**
     * Construct a ExpressionMatchTests instance
     */
    ExpressionMatchTests() {}

    /**
     * Tests ExpressionMatch empty pattern handling
     */
    static void runTestsEmptyPattern();

    /**
     * Tests ExpressionMatch MatchMode::Phrase
     */
    static void runTestsMatchPhrase();

    /**
     * Tests ExpressionMatch MatchMode::MultiPhrase
     */
    static void runTestsMatchMultiPhrase();

    /**
     * Tests ExpressionMatch MatchMode::Wildcard
     */
    static void runTestsMatchWildcard();

    /**
     * Tests ExpressionMatch MatchMode::MultiWildcard
     */
    static void runTestsMatchMultiWildcard();

    /**
     * Tests ExpressionMatch MatchMode::RegEx
     */
    static void runTestsMatchRegEx();

    /**
     * Tests ExpressionMatch MultiWildcard whitespace trimming
     */
    static void runTestsTrimMultiWildcardWhitespace();

};
