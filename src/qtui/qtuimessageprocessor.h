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

#ifndef QTUIMESSAGEPROCESSOR_H_
#define QTUIMESSAGEPROCESSOR_H_

#include <utility>

#include <QTimer>

#include "abstractmessageprocessor.h"
#include "expressionmatch.h"
#include "nickhighlightmatcher.h"

class QtUiMessageProcessor : public AbstractMessageProcessor
{
    Q_OBJECT

public:
    enum Mode
    {
        TimerBased,
        Concurrent
    };

    QtUiMessageProcessor(QObject* parent);

    inline bool isProcessing() const { return _processing; }
    inline Mode processMode() const { return _processMode; }

    void reset() override;

public slots:
    void process(Message& msg) override;
    void process(QList<Message>& msgs) override;

    /**
     * Network removed from system
     *
     * Handles cleaning up cache from stale networks.
     *
     * @param id Network ID of removed network
     */
    void networkRemoved(NetworkId id) override;

private slots:
    void processNextMessage();
    void nicksCaseSensitiveChanged(const QVariant& variant);
    void highlightListChanged(const QVariant& variant);
    void highlightNickChanged(const QVariant& variant);

private:
    /**
     * Individual highlight rule (legacy client-side version)
     */
    class LegacyHighlightRule
    {
    public:
        /**
         * Construct an empty highlight rule
         */
        LegacyHighlightRule() = default;

        /**
         * Construct a highlight rule with the given parameters
         *
         * @param contents         String representing a message contents expression to match
         * @param isRegEx          True if regular expression, otherwise false
         * @param isCaseSensitive  True if case sensitive, otherwise false
         * @param isEnabled        True if enabled, otherwise false
         * @param chanName         String representing a channel name expression to match
         */
        LegacyHighlightRule(QString contents, bool isRegEx, bool isCaseSensitive, bool isEnabled, QString chanName)
            : _contents(std::move(contents))
            , _isRegEx(isRegEx)
            , _isCaseSensitive(isCaseSensitive)
            , _isEnabled(isEnabled)
            , _chanName(std::move(chanName))
        {
            _cacheInvalid = true;
            // Cache expression matches on construction
            //
            // This provides immediate feedback on errors when loading the rule.  If profiling shows
            // this as a performance bottleneck, this can be removed in deference to caching on
            // first use.
            //
            // Inversely, if needed for validity checks, caching can be done on every update below
            // instead of on first use.
            determineExpressions();
        }

        /**
         * Gets the message contents this rule matches
         *
         * NOTE: Use HighlightRule::contentsMatcher() for performing matches
         *
         * CAUTION: For legacy reasons, "contents" doubles as the identifier for the ignore rule.
         * Duplicate entries are not allowed.
         *
         * @return String representing a phrase or expression to match
         */
        inline QString contents() const { return _contents; }
        /**
         * Sets the message contents this rule matches
         *
         * @param contents String representing a phrase or expression to match
         */
        inline void setContents(const QString& contents)
        {
            _contents = contents;
            _cacheInvalid = true;
        }

        /**
         * Gets if this is a regular expression rule
         *
         * @return True if regular expression, otherwise false
         */
        inline bool isRegEx() const { return _isRegEx; }
        /**
         * Sets if this rule is a regular expression rule
         *
         * @param isRegEx True if regular expression, otherwise false
         */
        inline void setIsRegEx(bool isRegEx)
        {
            _isRegEx = isRegEx;
            _cacheInvalid = true;
        }

        /**
         * Gets if this rule is case sensitive
         *
         * @return True if case sensitive, otherwise false
         */
        inline bool isCaseSensitive() const { return _isCaseSensitive; }
        /**
         * Sets if this rule is case sensitive
         *
         * @param isCaseSensitive True if case sensitive, otherwise false
         */
        inline void setIsCaseSensitive(bool isCaseSensitive)
        {
            _isCaseSensitive = isCaseSensitive;
            _cacheInvalid = true;
        }

        /**
         * Gets if this rule is enabled and active
         *
         * @return True if enabled, otherwise false
         */
        inline bool isEnabled() const { return _isEnabled; }
        /**
         * Sets if this rule is enabled and active
         *
         * @param isEnabled True if enabled, otherwise false
         */
        inline void setIsEnabled(bool isEnabled) { _isEnabled = isEnabled; }

        /**
         * Gets the channel name this rule matches
         *
         * NOTE: Use HighlightRule::chanNameMatcher() for performing matches
         *
         * @return String representing a phrase or expression to match
         */
        inline QString chanName() const { return _chanName; }
        /**
         * Sets the channel name this rule matches
         *
         * @param chanName String representing a phrase or expression to match
         */
        inline void setChanName(const QString& chanName)
        {
            _chanName = chanName;
            _cacheInvalid = true;
        }

        /**
         * Gets the expression matcher for the message contents, caching if needed
         *
         * @return Expression matcher to compare with message contents
         */
        inline ExpressionMatch contentsMatcher() const
        {
            if (_cacheInvalid) {
                determineExpressions();
            }
            return _contentsMatch;
        }

        /**
         * Gets the expression matcher for the channel name, caching if needed
         *
         * @return Expression matcher to compare with channel name
         */
        inline ExpressionMatch chanNameMatcher() const
        {
            if (_cacheInvalid) {
                determineExpressions();
            }
            return _chanNameMatch;
        }

        bool operator!=(const LegacyHighlightRule& other) const;

    private:
        /**
         * Update internal cache of expression matching if needed
         */
        void determineExpressions() const;

        QString _contents = {};
        bool _isRegEx = false;
        bool _isCaseSensitive = false;
        bool _isEnabled = true;
        QString _chanName = {};

        // These represent internal cache and should be safe to mutate in 'const' functions
        // See https://stackoverflow.com/questions/3141087/what-is-meant-with-const-at-end-of-function-declaration
        mutable bool _cacheInvalid = true;            ///< If true, match cache needs redone
        mutable ExpressionMatch _contentsMatch = {};  ///< Expression match cache for message content
        mutable ExpressionMatch _chanNameMatch = {};  ///< Expression match cache for channel name
    };

    using LegacyHighlightRuleList = QList<LegacyHighlightRule>;

    void checkForHighlight(Message& msg);
    void startProcessing();

    using HighlightNickType = NotificationSettings::HighlightNickType;

    LegacyHighlightRuleList _highlightRuleList;  ///< Custom highlight rule list
    NickHighlightMatcher _nickMatcher = {};      ///< Nickname highlight matcher

    /// Nickname highlighting mode
    HighlightNickType _highlightNick = HighlightNickType::CurrentNick;
    bool _nicksCaseSensitive = false;  ///< If true, match nicknames with exact case

    QList<QList<Message>> _processQueue;
    QList<Message> _currentBatch;
    QTimer _processTimer;
    bool _processing;
    Mode _processMode;
};

#endif
