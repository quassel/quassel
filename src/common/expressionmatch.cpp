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

#include "expressionmatch.h"

#include <QChar>
#include <QDebug>
#include <QString>
#include <QStringList>

ExpressionMatch::ExpressionMatch(const QString& expression, MatchMode mode, bool caseSensitive)
{
    // Store the original parameters for later reference
    _sourceExpression = expression;
    _sourceMode = mode;
    _sourceCaseSensitive = caseSensitive;

    // Calculate the internal regex
    //
    // Do this now instead of on-demand to provide immediate feedback on errors when editing
    // highlight and ignore rules.
    cacheRegEx();
}

bool ExpressionMatch::match(const QString& string, bool matchEmpty) const
{
    // Handle empty expression strings
    if (_sourceExpressionEmpty) {
        // Match found if matching empty is allowed, otherwise no match found
        return matchEmpty;
    }

    if (!isValid()) {
        // Can't match on an invalid rule
        return false;
    }

    // We have "_matchRegEx", "_matchInvertRegEx", or both due to isValid() check above

    // If specified, first check inverted rules
    if (_matchInvertRegExActive && _matchInvertRegEx.isValid()) {
        // Check inverted match rule
        if (_matchInvertRegEx.match(string).hasMatch()) {
            // Inverted rule matched, the rest of the rule cannot match
            return false;
        }
    }

    if (_matchRegExActive && _matchRegEx.isValid()) {
        // Check regular match rule
        return _matchRegEx.match(string).hasMatch();
    }
    else {
        // If no valid regular rules exist, due to the isValid() check there must be valid inverted
        // rules that did not match.  Count this as properly matching (implicit wildcard).
        return true;
    }
}

QString ExpressionMatch::trimMultiWildcardWhitespace(const QString& originalRule)
{
    // This gets handled in two steps:
    //
    // 1.  Break apart ";"-separated list into components
    // 2.  Combine whitespace-trimmed components into wildcard expression
    //
    // Let's start by making the list...

    // Convert a ";"-separated list into an actual list, splitting on newlines and unescaping
    // escaped characters

    // Escaped list rules (where "[\n]" represents newline):
    // ---------------
    // Token  | Outcome
    // -------|--------
    // ;      | Split
    // \;     | Keep as "\;"
    // \\;    | Split (keep as "\\")
    // \\\    | Keep as "\\" + "\", set consecutive slashes to 1
    // [\n]   | Split
    // \[\n]  | Split (keep as "\")
    // \\[\n] | Split (keep as "\\")
    // ...    | Keep as "..."
    // \...   | Keep as "\..."
    // \\...  | Keep as "\\..."
    //
    // Strings are forced to end with "\n", always applying "\..." and "\\..." rules
    // "..." also includes another "\" character
    //
    // All whitespace is trimmed from each component

    // "\\" and "\" are not downconverted to allow for other escape codes to be detected in
    // ExpressionMatch::wildcardToRegex

    // Example:
    //
    // > Wildcard rule
    // norm; norm-space ; newline-space [\n] ;escape \; sep ; slash-end-split\\; quad\\\\norm;
    // newline-split-slash\\[\n] slash-at-end\\                       [line does not continue]
    //
    // > Components
    //   norm
    //   norm-space
    //   newline-space
    //   escape \; sep
    //   slash-end-split\\          [line does not continue]
    //   quad\\\\norm
    //   newline-split-slash\\      [line does not continue]
    //   slash-at-end\\             [line does not continue]
    //
    // > Trimmed wildcard rule
    // norm; norm-space; newline-space[\n]escape \; sep; slash-end-split\\; quad\\\\norm;
    // newline-split-slash\\[\n]slash-at-end\\                        [line does not continue]
    //
    // (Newlines are encoded as "[\n]".  Ignore linebreaks for the sake of comment wrapping.)

    // Note: R"(\\)" results in the literal of "\\", two backslash characters.  Anything inside the
    // brackets is treated as a literal.  Outside the brackets but inside the quotes is still
    // escaped.
    //
    // See https://en.cppreference.com/w/cpp/language/string_literal

    // Prepare to loop!

    QString rule(originalRule);

    // Force a termination at the end of the string to trigger a split
    // Don't check for ";" splits as they may be escaped
    if (!rule.endsWith("\n")) {
        rule.append("\n");
    }

    // Result
    QString result = {};
    // Current character
    QChar curChar = {};
    // Current string
    QString curString = {};
    // Max length
    int sourceLength = rule.length();
    // Consecutive "\" characters
    int consecutiveSlashes = 0;

    // We know it's going to be the same length or smaller, so reserve the same size as the string
    result.reserve(sourceLength);

    // For every character...
    for (int i = 0; i < sourceLength; i++) {
        // Get the character
        curChar = rule.at(i);
        // Check if it's on the list of special list characters, converting to Unicode for use
        // in the switch statement
        //
        // See https://doc.qt.io/qt-5/qchar.html#unicode
        switch (curChar.unicode()) {
        case ';':
            // Separator found
            switch (consecutiveSlashes) {
            case 0:
            case 2:
                // ";"   -> Split
                // ...or...
                // "\\;" -> Split (keep as "\\")
                // Not escaped separator, split into a new item

                // Apply the additional "\\" if needed
                if (consecutiveSlashes == 2) {
                    // "\\;" -> Split (keep as "\\")
                    curString.append(R"(\\)");
                }

                // Remove any whitespace, e.g. "item1; item2" -> " item2" -> "item2"
                curString = curString.trimmed();

                // Skip empty items
                if (!curString.isEmpty()) {
                    // Add to list with the same separator used
                    result.append(curString + "; ");
                }
                // Reset the current list item
                curString.clear();
                break;
            case 1:
                // "\;" -> Keep as "\;"
                curString.append(R"(\;)");
                break;
            default:
                // This shouldn't ever happen (even with invalid wildcard rules), log a warning
                qWarning() << Q_FUNC_INFO << "Wildcard rule" << rule << "resulted in rule component" << curString
                           << "with unexpected count of consecutive '\\' (" << consecutiveSlashes << "), ignoring" << curChar
                           << "character!";
                break;
            }
            consecutiveSlashes = 0;
            break;
        case '\\':
            // Split escape
            // Increase consecutive slash count
            consecutiveSlashes++;
            // Check if we've reached "\\\"...
            if (consecutiveSlashes == 3) {
                // "\\\" -> Keep as "\\" + "\"
                curString.append(R"(\\)");
                // Set consecutive slashes to 1, recognizing the trailing "\"
                consecutiveSlashes = 1;
            }
            else if (consecutiveSlashes > 3) {
                // This shouldn't ever happen (even with invalid wildcard rules), log a warning
                qWarning() << Q_FUNC_INFO << "Wildcard rule" << rule << "resulted in rule component" << curString
                           << "with unexpected count of consecutive '\\' (" << consecutiveSlashes << "), ignoring" << curChar
                           << "character!";
                break;
            }
            break;
        case '\n':
            // Newline found
            // Preserve the characters as they are now

            // "[\n]"   -> Split
            // "\[\n]"  -> Split (keep as "\")
            // "\\[\n]" -> Split (keep as "\\")

            switch (consecutiveSlashes) {
            case 0:
                // Keep string as is
                break;
            case 1:
            case 2:
                // Apply the additional "\" or "\\"
                curString.append(QString(R"(\)").repeated(consecutiveSlashes));
                break;
            default:
                // This shouldn't ever happen (even with invalid wildcard rules), log a warning
                qWarning() << Q_FUNC_INFO << "Wildcard rule" << rule << "resulted in rule component" << curString
                           << "with unexpected count of consecutive '\\' (" << consecutiveSlashes << "), applying newline split anyways!";
                break;
            }

            // Remove any whitespace, e.g. "item1; item2" -> " item2" -> "item2"
            curString = curString.trimmed();

            // Skip empty items
            if (!curString.isEmpty()) {
                // Add to list with the same separator used
                result.append(curString + "\n");
            }
            // Reset the current list item
            curString.clear();
            consecutiveSlashes = 0;
            break;
        default:
            // Preserve the characters as they are now
            switch (consecutiveSlashes) {
            case 0:
                // "..."   -> Keep as "..."
                curString.append(curChar);
                break;
            case 1:
            case 2:
                // "\..."  -> Keep as "\..."
                // "\\..." -> Keep as "\\..."
                curString.append(QString("\\").repeated(consecutiveSlashes) + curChar);
                break;
            default:
                // This shouldn't ever happen (even with invalid wildcard rules), log a warning
                qWarning() << Q_FUNC_INFO << "Wildcard rule" << rule << "resulted in rule component" << curString
                           << "with unexpected count of consecutive '\\' (" << consecutiveSlashes << "), ignoring " << curChar
                           << "char escape!";
                break;
            }
            consecutiveSlashes = 0;
            break;
        }
    }

    // Remove any trailing separators
    if (result.endsWith("; ")) {
        result.chop(2);
    }

    // Remove any trailing whitespace
    return result.trimmed();
}

void ExpressionMatch::cacheRegEx()
{
    _matchRegExActive = false;
    _matchInvertRegExActive = false;

    _sourceExpressionEmpty = _sourceExpression.isEmpty();
    if (_sourceExpressionEmpty) {
        // No need to calculate anything for empty strings
        return;
    }

    // Convert the given expression to a regular expression based on the mode
    switch (_sourceMode) {
    case MatchMode::MatchPhrase:
        // Match entire phrase, noninverted
        // Don't trim whitespace for phrase matching as someone might want to match on " word ", a
        // more-specific request than "word".
        _matchRegEx = regExFactory("(?:^|\\W)" + regExEscape(_sourceExpression) + "(?:\\W|$)", _sourceCaseSensitive);
        _matchRegExActive = true;
        break;
    case MatchMode::MatchMultiPhrase:
        // Match multiple entire phrases, noninverted
        // Convert from multiple-phrase rules
        _matchRegEx = regExFactory(convertFromMultiPhrase(_sourceExpression), _sourceCaseSensitive);
        _matchRegExActive = true;
        break;
    case MatchMode::MatchWildcard:
        // Match as wildcard expression
        // Convert from wildcard rules for a single wildcard
        if (_sourceExpression.startsWith("!")) {
            // Inverted rule: take the remainder of the string
            // "^" + invertComponents.at(0) + "$"
            _matchInvertRegEx = regExFactory("^" + wildcardToRegEx(_sourceExpression.mid(1)) + "$", _sourceCaseSensitive);
            _matchInvertRegExActive = true;
        }
        else {
            // Normal rule: take the whole string
            // Account for any escaped "!" (i.e. "\!") by skipping past the "\", but don't skip past
            // escaped "\" (i.e. "\\!")
            _matchRegEx = regExFactory("^" + wildcardToRegEx(_sourceExpression.startsWith("\\!") ? _sourceExpression.mid(1) : _sourceExpression)
                                           + "$",
                                       _sourceCaseSensitive);
            _matchRegExActive = true;
        }
        break;
    case MatchMode::MatchMultiWildcard:
        // Match as multiple wildcard expressions
        // Convert from wildcard rules for multiple wildcards
        // (The generator function handles setting matchRegEx/matchInvertRegEx)
        generateFromMultiWildcard(_sourceExpression, _sourceCaseSensitive);
        break;
    case MatchMode::MatchRegEx:
        // Match as regular expression
        if (_sourceExpression.startsWith("!")) {
            // Inverted rule: take the remainder of the string
            _matchInvertRegEx = regExFactory(_sourceExpression.mid(1), _sourceCaseSensitive);
            _matchInvertRegExActive = true;
        }
        else {
            // Normal rule: take the whole string
            // Account for any escaped "!" (i.e. "\!") by skipping past the "\", but don't skip past
            // escaped "\" (i.e. "\\!")
            _matchRegEx = regExFactory(_sourceExpression.startsWith("\\!") ? _sourceExpression.mid(1) : _sourceExpression,
                                       _sourceCaseSensitive);
            _matchRegExActive = true;
        }
        break;
    default:
        // This should never happen if you keep the above consistent
        qWarning() << Q_FUNC_INFO << "Unknown MatchMode" << (int)_sourceMode << "!";
        break;
    }

    if (!_sourceExpressionEmpty && !isValid()) {
        // This can happen with invalid regex, so make it a bit more user-friendly.  Set it to Info
        // level as ideally someone's not just going to leave a broken match rule around.  For
        // MatchRegEx, they probably need to fix their regex rule.  For the other modes, there's
        // probably a bug in the parsing routines (which should also be fixed).
        qInfo() << "Could not parse expression match rule" << _sourceExpression << "(match mode:" << (int)_sourceMode
                 << "), this rule will be ignored";
    }
}

QRegularExpression ExpressionMatch::regExFactory(const QString& regExString, bool caseSensitive)
{
    // This is required, else extra-ASCII codepoints get treated as word boundaries
    QRegularExpression::PatternOptions options = QRegularExpression::UseUnicodePropertiesOption;

    if (!caseSensitive) {
        options |= QRegularExpression::CaseInsensitiveOption;
    }

    QRegularExpression newRegEx = QRegularExpression(regExString, options);

    // Check if rule is valid
    if (!newRegEx.isValid()) {
        // This can happen with invalid regex, so make it a bit more user-friendly.  Keep this
        // distinct from the main info-level message for easier debugging in case a regex component
        // in Wildcard or Phrase mode breaks.
        qDebug() << "Internal regular expression component" << regExString << "is invalid and will be ignored";
    }
    // Qt offers explicit control over when QRegularExpression objects get optimized.
    // By default, patterns are only optimized after some number of uses as defined
    // within Qt internals.
    //
    // In the context of ExpressionMatch, some regular expressions might go unused, e.g. a highlight
    // rule might never match a channel pattern, resulting in the contents pattern being untouched.
    // It should be safe to let Qt handle optimization, taking a non-deterministic, one-off
    // performance penalty on optimization for the sake of saving memory usage on patterns that
    // don't get used.
    //
    // If profiling shows expressions are generally used and/or the automatic optimization
    // interferes incurs too high of a penalty (unlikely given we've created regular expression
    // objects willy-nilly before now), this can be revisited to explicitly call...
    //
    // else {
    //     // Optimize regex now
    //     newRegEx.optimize();
    // }
    //
    // NOTE: This should only be called if the expression is valid!  Apply within an "else" of the
    // inverted isValid() check above.
    //
    // See https://doc.qt.io/qt-5/qregularexpression.html#optimize

    return newRegEx;
}

QString ExpressionMatch::regExEscape(const QString& phrase)
{
    // Escape the given phrase of any special regular expression characters
    return QRegularExpression::escape(phrase);
}

QString ExpressionMatch::convertFromMultiPhrase(const QString& originalRule)
{
    // Convert the multi-phrase rule into regular expression format
    // Split apart the original rule into components
    // Use QStringList instead of std::vector<QString> to make use of Qt's built-in .join() method
    QStringList components = {};
    // Split on "\n"
    for (auto&& component : originalRule.split("\n", QString::SkipEmptyParts)) {
        // Don't trim whitespace to maintain consistency with single phrase matching
        // As trimming is not performed, empty components will already be skipped.  This means " "
        // is considered a valid matching phrase.

        // Take the whole string, escaping any regex
        components.append(regExEscape(component));
    }

    // Create full regular expression by...
    // > Enclosing within a non-capturing group to avoid overhead of text extraction, "(?:...)"
    // > Flattening normal and inverted rules using the regex OR character "...|..."
    //
    // Before: [foo, bar, baz]
    // After:  (?:^|\W)(?:foo|bar|baz)(?:\W|$)

    if (components.count() == 1) {
        // Single item, skip the noncapturing group
        return "(?:^|\\W)" + components.at(0) + "(?:\\W|$)";
    }
    else {
        return "(?:^|\\W)(?:" + components.join("|") + ")(?:\\W|$)";
    }
}

void ExpressionMatch::generateFromMultiWildcard(const QString& originalRule, bool caseSensitive)
{
    // Convert the wildcard rule into regular expression format
    // First, reset the existing match expressions
    _matchRegEx = {};
    _matchInvertRegEx = {};
    _matchRegExActive = false;
    _matchInvertRegExActive = false;

    // This gets handled in three steps:
    //
    // 1.  Break apart ";"-separated list into components
    // 2.  Convert components from wildcard format into regular expression format
    // 3.  Combine normal/invert components into normal/invert regular expressions
    //
    // Let's start by making the list...

    // Convert a ";"-separated list into an actual list, splitting on newlines and unescaping
    // escaped characters

    // Escaped list rules (where "[\n]" represents newline):
    // ---------------
    // Token  | Outcome
    // -------|--------
    // ;      | Split
    // \;     | Replace with ";"
    // \\;    | Split (keep as "\\")
    // !      | At start: mark as inverted
    // \!     | At start: replace with "!"
    // \\!    | At start: keep as "\\!" (replaced with "\!" in wildcard conversion)
    // !      | Elsewhere: keep as "!"
    // \!     | Elsewhere: keep as "\!"
    // \\!    | Elsewhere: keep as "\\!"
    // \\\    | Keep as "\\" + "\", set consecutive slashes to 1
    // [\n]   | Split
    // \[\n]  | Split (keep as "\")
    // \\[\n] | Split (keep as "\\")
    // ...    | Keep as "..."
    // \...   | Keep as "\..."
    // \\...  | Keep as "\\..."
    //
    // Strings are forced to end with "\n", always applying "\..." and "\\..." rules
    // "..." also includes another "\" character
    //
    // All whitespace is trimmed from each component

    // "\\" and "\" are not downconverted to allow for other escape codes to be detected in
    // ExpressionMatch::wildcardToRegex

    // Example:
    //
    // > Wildcard rule
    // norm;!invert; norm-space ; !invert-space ;;!;\!norm-escaped;\\!slash-invert;\\\\double;
    // escape\;sep;slash-end-split\\;quad\\\\!noninvert;newline-split[\n]newline-split-slash\\[\n]
    // slash-at-end\\               [line does not continue]
    //
    // (Newlines are encoded as "[\n]".  Ignore linebreaks for the sake of comment wrapping.)
    //
    //
    // > Normal components without wildcard conversion
    //   norm
    //   norm-space
    //   !norm-escaped
    //   \\!slash-invert
    //   \\\\double
    //   escape;sep
    //   slash-end-split\\          [line does not continue]
    //   quad\\\\!noninvert
    //   newline-split
    //   newline-split-slash\\      [line does not continue]
    //   slash-at-end\\             [line does not continue]
    //
    // > Inverted components without wildcard conversion
    //   invert
    //   invert-space
    //
    //
    // > Normal components with wildcard conversion
    //   norm
    //   norm\-space
    //   \!norm\-escaped
    //   \\\!slash\-invert
    //   \\\\double
    //   escape\;sep
    //   slash\-end\-split\\        [line does not continue]
    //   quad\\\\\!noninvert
    //   newline\-split
    //   newline\-split\-slash\\    [line does not continue]
    //   slash\-at\-end\\           [line does not continue]
    //
    // > Inverted components with wildcard conversion
    //   invert
    //   invert\-space
    //
    //
    // > Normal wildcard-converted regex
    // ^(?:norm|norm\-space|\!norm\-escaped|\\\!slash\-invert|\\\\double|escape\;sep|
    // slash\-end\-split\\|quad\\\\\!noninvert|newline\-split|newline\-split\-slash\\|
    // slash\-at\-end\\)$
    //
    // > Inverted wildcard-converted regex
    // ^(?:invert|invert\-space)$

    // Note: R"(\\)" results in the literal of "\\", two backslash characters.  Anything inside the
    // brackets is treated as a literal.  Outside the brackets but inside the quotes is still
    // escaped.
    //
    // See https://en.cppreference.com/w/cpp/language/string_literal

    // Prepare to loop!

    QString rule(originalRule);

    // Force a termination at the end of the string to trigger a split
    // Don't check for ";" splits as they may be escaped
    if (!rule.endsWith("\n")) {
        rule.append("\n");
    }

    // Result, sorted into normal and inverted rules
    // Use QStringList instead of std::vector<QString> to make use of Qt's built-in .join() method
    QStringList normalComponents = {}, invertComponents = {};

    // Current character
    QChar curChar = {};
    // Current string
    QString curString = {};
    // Max length
    int sourceLength = rule.length();
    // Consecutive "\" characters
    int consecutiveSlashes = 0;
    // Whether or not this marks an inverted rule
    bool isInverted = false;
    // Whether or not we're at the beginning of the rule (for detecting "!" and "\!")
    bool isRuleStart = true;

    // We know it's going to have ";"-count items or less, so reserve ";"-count items for both.
    // Without parsing it's not easily possible to tell which are escaped or not, and among the
    // non-escaped entries, which are inverted or not.  These get destroyed once out of scope of
    // this function, so balancing towards performance over memory usage should be okay, hopefully.
    int separatorCount = rule.count(";");
    normalComponents.reserve(separatorCount);
    invertComponents.reserve(separatorCount);

    // For every character...
    for (int i = 0; i < sourceLength; i++) {
        // Get the character
        curChar = rule.at(i);
        // Check if it's on the list of special list characters, converting to Unicode for use
        // in the switch statement
        //
        // See https://doc.qt.io/qt-5/qchar.html#unicode
        switch (curChar.unicode()) {
        case ';':
            // Separator found
            switch (consecutiveSlashes) {
            case 0:
            case 2:
                // ";"   -> Split
                // ...or...
                // "\\;" -> Split (keep as "\\")
                // Not escaped separator, split into a new item

                // Apply the additional "\\" if needed
                if (consecutiveSlashes == 2) {
                    // "\\;" -> Split (keep as "\\")
                    curString.append(R"(\\)");
                }

                // Remove any whitespace, e.g. "item1; item2" -> " item2" -> "item2"
                curString = curString.trimmed();

                // Skip empty items
                if (!curString.isEmpty()) {
                    // Add to inverted/normal list
                    if (isInverted) {
                        invertComponents.append(wildcardToRegEx(curString));
                    }
                    else {
                        normalComponents.append(wildcardToRegEx(curString));
                    }
                }
                // Reset the current list item
                curString.clear();
                isInverted = false;
                isRuleStart = true;
                break;
            case 1:
                // "\;" -> Replace with ";"
                curString.append(";");
                isRuleStart = false;
                break;
            default:
                // This shouldn't ever happen (even with invalid wildcard rules), log a warning
                qWarning() << Q_FUNC_INFO << "Wildcard rule" << rule << "resulted in rule component" << curString
                           << "with unexpected count of consecutive '\\' (" << consecutiveSlashes << "), ignoring" << curChar
                           << "character!";
                isRuleStart = false;
                break;
            }
            consecutiveSlashes = 0;
            break;
        case '!':
            // Rule inverter found
            if (isRuleStart) {
                // Apply the inverting logic
                switch (consecutiveSlashes) {
                case 0:
                    // "!"   -> At start: mark as inverted
                    isInverted = true;
                    // Don't include the "!" character
                    break;
                case 1:
                    // "\!"  -> At start: replace with "!"
                    curString.append("!");
                    break;
                case 2:
                    // "\\!" -> At start: keep as "\\!" (replaced with "\!" in wildcard conversion)
                    curString.append(R"(\\!)");
                    break;
                default:
                    // This shouldn't ever happen (even with invalid wildcard rules), log a warning
                    qWarning() << Q_FUNC_INFO << "Wildcard rule" << rule << "resulted in rule component" << curString
                               << "with unexpected count of consecutive '\\' (" << consecutiveSlashes << "), ignoring" << curChar
                               << "character!";
                    break;
                }
            }
            else {
                // Preserve the characters as they are now
                switch (consecutiveSlashes) {
                case 0:
                    // "!"    -> Elsewhere: keep as "!"
                    curString.append("!");
                    break;
                case 1:
                case 2:
                    // "\!"  -> Elsewhere: keep as "\!"
                    // "\\!" -> Elsewhere: keep as "\\!"
                    curString.append(QString(R"(\)").repeated(consecutiveSlashes) + "!");
                    break;
                default:
                    // This shouldn't ever happen (even with invalid wildcard rules), log a warning
                    qWarning() << Q_FUNC_INFO << "Wildcard rule" << rule << "resulted in rule component" << curString
                               << "with unexpected count of consecutive '\\' (" << consecutiveSlashes << "), ignoring" << curChar
                               << "character!";
                    break;
                }
            }
            isRuleStart = false;
            consecutiveSlashes = 0;
            break;
        case '\\':
            // Split escape
            // Increase consecutive slash count
            consecutiveSlashes++;
            // Check if we've reached "\\\"...
            if (consecutiveSlashes == 3) {
                // "\\\" -> Keep as "\\" + "\"
                curString.append(R"(\\)");
                // No longer at the rule start
                isRuleStart = false;
                // Set consecutive slashes to 1, recognizing the trailing "\"
                consecutiveSlashes = 1;
            }
            else if (consecutiveSlashes > 3) {
                // This shouldn't ever happen (even with invalid wildcard rules), log a warning
                qWarning() << Q_FUNC_INFO << "Wildcard rule" << rule << "resulted in rule component" << curString
                           << "with unexpected count of consecutive '\\' (" << consecutiveSlashes << "), ignoring" << curChar
                           << "character!";
                break;
            }
            // Don't set "isRuleStart" here as "\" is used in escape sequences
            break;
        case '\n':
            // Newline found
            // Preserve the characters as they are now

            // "[\n]"   -> Split
            // "\[\n]"  -> Split (keep as "\")
            // "\\[\n]" -> Split (keep as "\\")

            switch (consecutiveSlashes) {
            case 0:
                // Keep string as is
                break;
            case 1:
            case 2:
                // Apply the additional "\" or "\\"
                curString.append(QString(R"(\)").repeated(consecutiveSlashes));
                break;
            default:
                // This shouldn't ever happen (even with invalid wildcard rules), log a warning
                qWarning() << Q_FUNC_INFO << "Wildcard rule" << rule << "resulted in rule component" << curString
                           << "with unexpected count of consecutive '\\' (" << consecutiveSlashes << "), applying newline split anyways!";
                break;
            }

            // Remove any whitespace, e.g. "item1; item2" -> " item2" -> "item2"
            curString = curString.trimmed();

            // Skip empty items
            if (!curString.isEmpty()) {
                // Add to inverted/normal list
                if (isInverted) {
                    invertComponents.append(wildcardToRegEx(curString));
                }
                else {
                    normalComponents.append(wildcardToRegEx(curString));
                }
            }
            // Reset the current list item
            curString.clear();
            isInverted = false;
            isRuleStart = true;
            consecutiveSlashes = 0;
            break;
        default:
            // Preserve the characters as they are now
            switch (consecutiveSlashes) {
            case 0:
                // "..."   -> Keep as "..."
                curString.append(curChar);
                break;
            case 1:
            case 2:
                // "\..."  -> Keep as "\..."
                // "\\..." -> Keep as "\\..."
                curString.append(QString("\\").repeated(consecutiveSlashes) + curChar);
                break;
            default:
                // This shouldn't ever happen (even with invalid wildcard rules), log a warning
                qWarning() << Q_FUNC_INFO << "Wildcard rule" << rule << "resulted in rule component" << curString
                           << "with unexpected count of consecutive '\\' (" << consecutiveSlashes << "), ignoring " << curChar
                           << "char escape!";
                break;
            }
            // Don't mark as past rule start for whitespace (whitespace gets trimmed)
            if (!curChar.isSpace()) {
                isRuleStart = false;
            }
            consecutiveSlashes = 0;
            break;
        }
    }

    // Clean up any duplicates
    normalComponents.removeDuplicates();
    invertComponents.removeDuplicates();

    // Create full regular expressions by...
    // > Anchoring to start and end of string to mimic QRegExp's .exactMatch() handling, "^...$"
    // > Enclosing within a non-capturing group to avoid overhead of text extraction, "(?:...)"
    // > Flattening normal and inverted rules using the regex OR character "...|..."
    //
    // Before: [foo, bar, baz]
    // After:  ^(?:foo|bar|baz)$
    //
    // See https://doc.qt.io/qt-5/qregularexpression.html#porting-from-qregexp-exactmatch
    // And https://regex101.com/

    // Any empty/invalid regex are handled within ExpressionMatch::match()
    if (!normalComponents.isEmpty()) {
        // Create normal match regex
        if (normalComponents.count() == 1) {
            // Single item, skip the noncapturing group
            _matchRegEx = regExFactory("^" + normalComponents.at(0) + "$", caseSensitive);
        }
        else {
            _matchRegEx = regExFactory("^(?:" + normalComponents.join("|") + ")$", caseSensitive);
        }
        _matchRegExActive = true;
    }
    if (!invertComponents.isEmpty()) {
        // Create invert match regex
        if (invertComponents.count() == 1) {
            // Single item, skip the noncapturing group
            _matchInvertRegEx = regExFactory("^" + invertComponents.at(0) + "$", caseSensitive);
        }
        else {
            _matchInvertRegEx = regExFactory("^(?:" + invertComponents.join("|") + ")$", caseSensitive);
        }
        _matchInvertRegExActive = true;
    }
}

QString ExpressionMatch::wildcardToRegEx(const QString& expression)
{
    // Convert the wildcard expression into regular expression format

    // We're taking a little bit different of a route...
    //
    // Original QRegExp::Wildcard rules:
    // --------------------------
    // Wildcard | Regex | Outcome
    // ---------|-------|--------
    // *        | .*    | zero or more of any character
    // ?        | .     | any single character
    //
    // NOTE 1: This is QRegExp::Wildcard, not QRegExp::WildcardUnix
    //
    // NOTE 2: We are ignoring the "[...]" character-class matching functionality of
    // QRegExp::Wildcard as that feature's a bit more complex and can be handled with full-featured
    // regexes.
    //
    // See https://doc.qt.io/qt-5/qregexp.html#wildcard-matching
    //
    // Quassel originally did not use QRegExp::WildcardUnix, which prevented escaping "*" and "?" in
    // messages.  Unfortunately, spam messages might decide to use both, so offering a way to escape
    // makes sense.
    //
    // On the flip-side, that means to match "\" requires escaping as "\\", breaking backwards
    // compatibility.
    //
    // Quassel's Wildcard rules
    // ------------------------------------------
    // Wildcard | Regex escaped | Regex | Outcome
    // ---------|---------------|-------|--------
    // *        | \*            | .*    | zero or more of any character
    // ?        | \?            | .     | any single character
    // \*       | \\\*          | \*    | literal "*"
    // \?       | \\\?          | \?    | literal "?"
    // \[...]   | \\[...]       | [...] | invalid escape, ignore it
    // \\       | \\\\          | \\    | literal "\"
    //
    // In essence, "*" and "?" need changed only if not escaped, "\\" collapses into "\", "\" gets
    // ignored; other characters escape normally.
    //
    // Example:
    //
    // > Wildcard rule
    // never?gonna*give\*you\?up\\test|y\yeah\\1\\\\2\\\1inval
    //
    // ("\\\\" represents "\\", "\\" represents "\", and "\\\" is valid+invalid, "\")
    //
    // > Regex escaped wildcard rule
    // never\?gonna\*give\\\*you\\\?up\\\\test\|y\\yeah\\\\1\\\\\\\\2\\\\\\1inval
    //
    // > Expected correct regex
    // never.gonna.*give\*you\?up\\test\|yyeah\\1\\\\2\\1inval
    //
    // > Undoing regex escaping of "\" as "\\" (i.e. simple replace, with special escapes intact)
    // never.gonna.*give\*you\?up\test\|yyeah\1\\2\1inval

    // Escape string according to regex
    QString regExEscaped(regExEscape(expression));

    // Fix up the result
    //
    // NOTE: In theory, regular expression lookbehind could solve this.  Unfortunately, QRegExp does
    // not support lookbehind, and it's theoretically inefficient, anyways.  Just use an approach
    // similar to that taken by QRegExp's official wildcard mode.
    //
    // Lookbehind example (that we can't use):
    // (?<!abc)test    Negative lookbehind - don't match if "test" is proceeded by "abc"
    //
    // See https://code.qt.io/cgit/qt/qtbase.git/tree/src/corelib/tools/qregexp.cpp
    //
    // NOTE: We don't copy QRegExp's mode as QRegularExpression has more special characters.  We
    // can't use the same escaping code, hence calling the appropriate QReg[...]::escape() above.

    // Prepare to loop!

    // Result
    QString result = {};
    // Current character
    QChar curChar = {};
    // Max length
    int sourceLength = regExEscaped.length();
    // Consecutive "\" characters
    int consecutiveSlashes = 0;

    // We know it's going to be the same length or smaller, so reserve the same size as the string
    result.reserve(sourceLength);

    // For every character...
    for (int i = 0; i < sourceLength; i++) {
        // Get the character
        curChar = regExEscaped.at(i);
        // Check if it's on the list of special wildcard characters, converting to Unicode for use
        // in the switch statement
        //
        // See https://doc.qt.io/qt-5/qchar.html#unicode
        switch (curChar.unicode()) {
        case '?':
            // Wildcard "?"
            switch (consecutiveSlashes) {
            case 1:
                // "?" -> "\?" -> "."
                // Convert from regex escaped "?" to regular expression
                result.append(".");
                break;
            case 3:
                // "\?" -> "\\\?" -> "\?"
                // Convert from regex escaped "\?" to literal string
                result.append(R"(\?)");
                break;
            default:
                // This shouldn't ever happen (even with invalid wildcard rules), log a warning
                qWarning() << Q_FUNC_INFO << "Wildcard rule" << expression << "resulted in escaped regular expression string"
                           << regExEscaped << " with unexpected count of consecutive '\\' (" << consecutiveSlashes << "), ignoring"
                           << curChar << "character!";
                break;
            }
            consecutiveSlashes = 0;
            break;
        case '*':
            // Wildcard "*"
            switch (consecutiveSlashes) {
            case 1:
                // "*" -> "\*" -> ".*"
                // Convert from regex escaped "*" to regular expression
                result.append(".*");
                break;
            case 3:
                // "\*" -> "\\\*" -> "\*"
                // Convert from regex escaped "\*" to literal string
                result.append(R"(\*)");
                break;
            default:
                // This shouldn't ever happen (even with invalid wildcard rules), log a warning
                qWarning() << Q_FUNC_INFO << "Wildcard rule" << expression << "resulted in escaped regular expression string"
                           << regExEscaped << " with unexpected count of consecutive '\\' (" << consecutiveSlashes << "), ignoring"
                           << curChar << "character!";
                break;
            }
            consecutiveSlashes = 0;
            break;
        case '\\':
            // Wildcard escape
            // Increase consecutive slash count
            consecutiveSlashes++;
            // Check if we've hit an escape sequence
            if (consecutiveSlashes == 4) {
                // "\\" -> "\\\\" -> "\\"
                // Convert from regex escaped "\\" to literal string
                result.append(R"(\\)");
                // Reset slash count
                consecutiveSlashes = 0;
            }
            break;
        default:
            // Any other character
            switch (consecutiveSlashes) {
            case 0:
            case 2:
                // "[...]"  -> "[...]"   -> "[...]"
                // ...or...
                // "\[...]" -> "\\[...]" -> "[...]"
                // Either just print the character itself, or convert from regex-escaped invalid
                // wildcard escape sequence to the character itself
                //
                // Both mean doing nothing, the actual character [...] gets appended below
                break;
            case 1:
                // "[...]" -> "\[...]" -> "\"
                // Keep regex-escaped special character "[...]" as literal string
                // (Where "[...]" represents any non-wildcard regex special character)
                result.append(R"(\)");
                // The actual character [...] gets appended below
                break;
            default:
                // This shouldn't ever happen (even with invalid wildcard rules), log a warning
                qWarning() << Q_FUNC_INFO << "Wildcard rule" << expression << "resulted in escaped regular expression string"
                           << regExEscaped << " with unexpected count of consecutive '\\' (" << consecutiveSlashes << "), ignoring"
                           << curChar << "char escape!";
                break;
            }
            consecutiveSlashes = 0;
            // Add the character itself
            result.append(curChar);
            break;
        }
    }

    // Anchoring to simulate QRegExp::exactMatch() is handled in
    // ExpressionMatch::convertFromWildcard()
    return result;
}
