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

#pragma once

#include "common-export.h"

#include <QRegularExpression>
#include <QString>
#include <QStringList>

/**
 * Expression matcher with multiple modes of operation and automatic caching for performance
 */
class COMMON_EXPORT ExpressionMatch
{

public:
    /// Expression matching mode
    enum class MatchMode {
        MatchPhrase,        ///< Match phrase as specified, no special handling
        MatchMultiPhrase,   ///< Match phrase as specified, split on \n only
        MatchWildcard,      ///< Match wildcards, "!" at start inverts, "\" escapes
        MatchMultiWildcard, ///< Match wildcards, split ; or \n, "!" at start inverts, "\" escapes
        MatchRegEx          ///< Match as regular expression, "!..." invert regex, "\" escapes
    };

    /**
     * Construct an empty ExpressionMatch
     */
    ExpressionMatch() = default;

    /**
     * Construct an Expression match with the given parameters
     *
     * @param expression     A phrase, wildcard expression, or regular expression
     * @param mode
     * @parblock
     * Expression matching mode
     * @see ExpressionMatch::MatchMode
     * @endparblock
     * @param caseSensitive  If true, match case-sensitively, otherwise ignore case when matching
     */
    ExpressionMatch(const QString &expression, MatchMode mode, bool caseSensitive);

    /**
     * Check if the given string matches the stored expression
     *
     * @param string      String to check
     * @param matchEmpty  If true, always match when the expression is empty, otherwise never match
     * @return            True if match found, otherwise false
     */
    bool match(const QString &string, bool matchEmpty = false) const;

    /**
     * Gets if the source expression is empty
     *
     * @return True if source expression is empty, otherwise false
     */
    inline bool isEmpty() const { return (_sourceExpressionEmpty); }

    /**
     * Gets if the source expression and parameters resulted in a valid expression matcher
     *
     * @return True if given expression is valid, otherwise false
     */
    inline bool isValid() const {
        // Either this must be empty, or normal or inverted rules must be valid and active
        return (_sourceExpressionEmpty
                || (_matchRegExActive && _matchRegEx.isValid())
                || (_matchInvertRegExActive && _matchInvertRegEx.isValid()));
    }

    /**
     * Gets the original expression match string
     *
     * @return QString of the source expression match string
     */
    inline QString sourceExpression() const { return _sourceExpression; }

    /**
     * Sets the expression match string
     *
     * @param expression A phrase, wildcard expression, or regular expression
     */
    void setSourceExpression(const QString &expression) {
        if (_sourceExpression != expression) {
            _sourceExpression = expression;
            cacheRegEx();
        }
    }

    /**
     * Gets the original expression match mode
     *
     * @return MatchMode of the source expression
     */
    inline MatchMode sourceMode() const { return _sourceMode; }

    /**
     * Sets the expression match mode
     *
     * @param mode
     * @parblock
     * Expression matching mode
     * @see ExpressionMatch::MatchMode
     * @endparblock
     */
    void setSourceMode(MatchMode mode) {
        if (_sourceMode != mode) {
            _sourceMode = mode;
            cacheRegEx();
        }
    }

    /**
     * Gets the original expression case-sensitivity
     *
     * @return True if case-sensitive, otherwise false
     */
    inline bool sourceCaseSensitive() const { return _sourceCaseSensitive; }

    /**
     * Sets the expression match as case sensitive or not
     *
     * @param caseSensitive If true, match case-sensitively, otherwise ignore case when matching
     */
    void setSourceCaseSensitive(bool caseSensitive) {
        if (_sourceCaseSensitive != caseSensitive) {
            _sourceCaseSensitive = caseSensitive;
            cacheRegEx();
        }
    }

    bool operator!=(const ExpressionMatch &other) const
    {
        return (_sourceExpression != other._sourceExpression ||
                _sourceMode != other._sourceMode ||
                _sourceCaseSensitive != other._sourceCaseSensitive);
    }

    /**
     * Trim extraneous whitespace from individual rules within a given MultiWildcard expression
     *
     * This respects the ";" escaping rules with "\".  It is safe to call this multiple times; a
     * trimmed string should remain unchanged.
     *
     * @see ExpressionMatch::MatchMode::MatchMultiWildcard
     *
     * @param originalRule MultiWildcard rule list, ";"-separated
     * @return Trimmed MultiWildcard rule list
     */
    static QString trimMultiWildcardWhitespace(const QString &originalRule);

private:
    /**
     * Calculates internal regular expressions
     *
     * Will always run when called, no cache validity checks performed.
     */
    void cacheRegEx();

    /**
     * Creates a regular expression object of appropriate type and case-sensitivity
     *
     * @param regExString    Regular expression string
     * @param caseSensitive  If true, match case-sensitively, otherwise ignore case when matching
     * @return Configured QRegularExpression
     */
    static QRegularExpression regExFactory(const QString &regExString, bool caseSensitive);

    /**
     * Escapes any regular expression characters in a string so they have no special meaning
     *
     * @param phrase String containing potential regular expression special characters
     * @return QString with all regular expression characters escaped
     */
    static QString regExEscape(const QString &phrase);

    /**
     * Converts a multiple-phrase rule into a regular expression
     *
     * Unconditionally splits phrases on "\n", whitespace is preserved
     *
     * @param originalRule MultiPhrase rule list, "\n"-separated
     * @return A regular expression matching the given phrases
     */
    static QString convertFromMultiPhrase(const QString &originalRule);

    /**
     * Internally converts a wildcard rule into regular expressions
     *
     * Splits wildcards on ";" and "\n", "!..." inverts section, "\" escapes
     *
     * @param originalRule   MultiWildcard rule list, ";"-separated
     * @param caseSensitive  If true, match case-sensitively, otherwise ignore case when matching
     */
    void generateFromMultiWildcard(const QString &originalRule, bool caseSensitive);

    /**
     * Converts a wildcard expression into a regular expression
     *
     * NOTE:  Does not handle Quassel's extended scope matching and splitting.
     *
     * @see ExpressionMatch::convertFromWildcard()
     * @return QString with all regular expression characters escaped
     */
    static QString wildcardToRegEx(const QString &expression);

    // Original/source components
    QString _sourceExpression = {};                  ///< Expression match string given on creation
    MatchMode _sourceMode = MatchMode::MatchPhrase;  ///< Expression match mode given on creation
    bool _sourceCaseSensitive = false;               ///< Expression case sensitive on creation

    // Derived components
    bool _sourceExpressionEmpty = false;             ///< Cached expression match string is empty

    /// Underlying regular expression matching instance for normal (noninverted) rules
    QRegularExpression _matchRegEx = {};
    bool _matchRegExActive = false;                  ///< If true, use normal expression in matching

    /// Underlying regular expression matching instance for inverted rules
    QRegularExpression _matchInvertRegEx = {};
    bool _matchInvertRegExActive = false;            ///< If true, use invert expression in matching
};
