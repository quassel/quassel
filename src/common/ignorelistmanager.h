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

#pragma once

#include "common-export.h"

#include <utility>

#include <QRegExp>
#include <QString>
#include <QStringList>

#include "expressionmatch.h"
#include "message.h"
#include "syncableobject.h"

class COMMON_EXPORT IgnoreListManager : public SyncableObject
{
    Q_OBJECT
    SYNCABLE_OBJECT

public:
    inline IgnoreListManager(QObject* parent = nullptr)
        : SyncableObject(parent)
    {
        setAllowClientUpdates(true);
    }

    enum IgnoreType
    {
        SenderIgnore,
        MessageIgnore,
        CtcpIgnore
    };

    enum StrictnessType
    {
        UnmatchedStrictness = 0,
        SoftStrictness = 1,
        HardStrictness = 2
    };

    enum ScopeType
    {
        GlobalScope,
        NetworkScope,
        ChannelScope,
    };

    /**
     * Individual ignore list rule
     */
    class COMMON_EXPORT IgnoreListItem
    {
    public:
        /**
         * Construct an empty ignore rule
         */
        IgnoreListItem() = default;

        /**
         * Construct an ignore rule with the given parameters
         *
         * CAUTION: For legacy reasons, "contents" doubles as the identifier for the ignore rule.
         * Duplicate entries are not allowed.
         *
         * @param type             Type of ignore rule
         * @param contents         String representing a message contents expression to match
         * @param isRegEx          True if regular expression, otherwise false
         * @param strictness       Strictness of ignore rule
         * @param scope            What to match scope rule against
         * @param scopeRule        String representing a scope rule expression to match
         * @param isEnabled        True if enabled, otherwise false
         */
        IgnoreListItem(
            IgnoreType type, QString contents, bool isRegEx, StrictnessType strictness, ScopeType scope, QString scopeRule, bool isEnabled)
            : _contents(std::move(contents))
            , _isRegEx(isRegEx)
            , _strictness(strictness)
            , _scope(scope)
            , _scopeRule(std::move(scopeRule))
            , _isEnabled(isEnabled)
        {
            // Allow passing empty "contents" as they can happen when editing an ignore rule

            // Handle CTCP ignores
            setType(type);

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
         * Gets the type of this ignore rule
         *
         * @return IgnoreType of the rule
         */
        inline IgnoreType type() const { return _type; }
        /**
         * Sets the type of this ignore rule
         *
         * @param type IgnoreType of the rule
         */
        inline void setType(IgnoreType type)
        {
            // Handle CTCP ignores
            if (type == CtcpIgnore) {
                // This is not performance-intensive; sticking with QRegExp for Qt 4 is fine
                // Split based on whitespace characters
                QStringList split(contents().split(QRegExp("\\s+"), QString::SkipEmptyParts));
                // Match on the first item, handling empty rules/matches
                if (!split.isEmpty()) {
                    // Take the first item as the sender
                    _cacheCtcpSender = split.takeFirst();
                    // Track the rest as CTCP types to ignore
                    _cacheCtcpTypes = split;
                }
                else {
                    // No match found - this can happen if a pure whitespace CTCP ignore rule is
                    // created.  Fall back to matching all senders.
                    if (_isRegEx) {
                        // RegEx match everything
                        _cacheCtcpSender = ".*";
                    }
                    else {
                        // Wildcard match everything
                        _cacheCtcpSender = "*";
                    }
                    // Clear the types (split is already empty)
                    _cacheCtcpTypes = split;
                }
            }
            _type = type;
        }

        /**
         * Gets the message contents this rule matches
         *
         * NOTE: Use IgnoreListItem::contentsMatcher() for performing matches
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
            // Allow passing empty "contents" as they can happen when editing an ignore rule
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
         * Gets the strictness of this ignore rule
         *
         * @return StrictnessType of the rule
         */
        inline StrictnessType strictness() const { return _strictness; }
        /**
         * Sets the strictness of this ignore rule
         *
         * @param strictness StrictnessType of the rule
         */
        inline void setStrictness(StrictnessType strictness) { _strictness = strictness; }

        /**
         * Gets what to match scope rule against
         *
         * @return ScopeType of the rule
         */
        inline ScopeType scope() const { return _scope; }
        /**
         * Sets what to match scope rule against
         *
         * @param type ScopeType of the rule
         */
        inline void setScope(ScopeType scope) { _scope = scope; }

        /**
         * Gets the scope rule this rule matches
         *
         * NOTE: Use IgnoreListItem::scopeRuleMatcher() for performing matches
         *
         * @return String representing a phrase or expression to match
         */
        inline QString scopeRule() const { return _scopeRule; }
        /**
         * Sets the scope rule this rule matches
         *
         * @param scopeRule String representing a phrase or expression to match
         */
        inline void setScopeRule(const QString& scopeRule)
        {
            _scopeRule = scopeRule;
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
         * Gets the ignored CTCP types for CTCP ignores
         *
         * @return List of CTCP types to ignore, or empty for all
         */
        inline QStringList ctcpTypes() const { return _cacheCtcpTypes; }

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
         * Gets the expression matcher for the scope, caching if needed
         *
         * @return Expression matcher to compare with scope
         */
        inline ExpressionMatch scopeRuleMatcher() const
        {
            if (_cacheInvalid) {
                determineExpressions();
            }
            return _scopeRuleMatch;
        }

        /**
         * Gets the expression matcher for the message contents, caching if needed
         *
         * @return Expression matcher to compare with message contents
         */
        inline ExpressionMatch senderCTCPMatcher() const
        {
            if (_cacheInvalid) {
                determineExpressions();
            }
            return _ctcpSenderMatch;
        }

        bool operator!=(const IgnoreListItem& other) const;

    private:
        /**
         * Update internal cache of expression matching if needed
         */
        void determineExpressions() const;

        IgnoreType _type = {};
        QString _contents = {};
        bool _isRegEx = false;
        StrictnessType _strictness = {};
        ScopeType _scope = {};
        QString _scopeRule = {};
        bool _isEnabled = true;

        QString _cacheCtcpSender = {};     ///< For CTCP rules, precalculate sender
        QStringList _cacheCtcpTypes = {};  ///< For CTCP rules, precalculate types

        // These represent internal cache and should be safe to mutate in 'const' functions
        // See https://stackoverflow.com/questions/3141087/what-is-meant-with-const-at-end-of-function-declaration
        mutable bool _cacheInvalid = true;              ///< If true, match cache needs redone
        mutable ExpressionMatch _contentsMatch = {};    ///< Expression match cache for message
        mutable ExpressionMatch _scopeRuleMatch = {};   ///< Expression match cache for scope rule
        mutable ExpressionMatch _ctcpSenderMatch = {};  ///< Expression match cache for CTCP nick
    };

    using IgnoreList = QList<IgnoreListItem>;

    int indexOf(const QString& ignore) const;
    inline bool contains(const QString& ignore) const { return indexOf(ignore) != -1; }
    inline bool isEmpty() const { return _ignoreList.isEmpty(); }
    inline int count() const { return _ignoreList.count(); }
    inline void removeAt(int index) { _ignoreList.removeAt(index); }
    inline IgnoreListItem& operator[](int i) { return _ignoreList[i]; }
    inline const IgnoreListItem& operator[](int i) const { return _ignoreList.at(i); }
    inline const IgnoreList& ignoreList() const { return _ignoreList; }

    //! Check if a message matches the IgnoreRule
    /** This method checks if a message matches the users ignorelist.
     * \param msg The Message that should be checked
     * \param network The networkname the message belongs to
     * \return UnmatchedStrictness, HardStrictness or SoftStrictness representing the match type
     */
    inline StrictnessType match(const Message& msg, const QString& network = QString())
    {
        return _match(msg.contents(), msg.sender(), msg.type(), network, msg.bufferInfo().bufferName());
    }

    bool ctcpMatch(const QString sender, const QString& network, const QString& type = QString());

    //  virtual void addIgnoreListItem(const IgnoreListItem &item);

public slots:
    virtual QVariantMap initIgnoreList() const;
    virtual void initSetIgnoreList(const QVariantMap& ignoreList);

    //! Request removal of an ignore rule based on the rule itself.
    /** Use this method if you want to remove a single ignore rule
     * and get that synced with the core immediately.
     * \param ignoreRule A valid ignore rule
     */
    virtual inline void requestRemoveIgnoreListItem(const QString& ignoreRule) { REQUEST(ARG(ignoreRule)) }
    virtual void removeIgnoreListItem(const QString& ignoreRule);

    //! Request toggling of "isActive" flag of a given ignore rule.
    /** Use this method if you want to toggle the "isActive" flag of a single ignore rule
     * and get that synced with the core immediately.
     * \param ignoreRule A valid ignore rule
     */
    virtual inline void requestToggleIgnoreRule(const QString& ignoreRule) { REQUEST(ARG(ignoreRule)) }
    virtual void toggleIgnoreRule(const QString& ignoreRule);

    //! Request an IgnoreListItem to be added to the ignore list
    /** Items added to the list with this method, get immediately synced with the core
     * \param type The IgnoreType of the new rule
     * \param ignoreRule The rule itself
     * \param isRegEx Signals if the rule should be interpreted as a regular expression
     * \param strictness Th StrictnessType that should be applied
     * \param scope The ScopeType that should be set
     * \param scopeRule A string of semi-colon separated network- or channelnames
     * \param isActive Signals if the rule is enabled or not
     */
    virtual inline void requestAddIgnoreListItem(
        int type, const QString& ignoreRule, bool isRegEx, int strictness, int scope, const QString& scopeRule, bool isActive)
    {
        REQUEST(ARG(type), ARG(ignoreRule), ARG(isRegEx), ARG(strictness), ARG(scope), ARG(scopeRule), ARG(isActive))
    }

    virtual void addIgnoreListItem(
        int type, const QString& ignoreRule, bool isRegEx, int strictness, int scope, const QString& scopeRule, bool isActive);

protected:
    void setIgnoreList(const QList<IgnoreListItem>& ignoreList) { _ignoreList = ignoreList; }

    StrictnessType _match(
        const QString& msgContents, const QString& msgSender, Message::Type msgType, const QString& network, const QString& bufferName);

signals:
    void ignoreAdded(IgnoreType type,
                     const QString& ignoreRule,
                     bool isRegex,
                     StrictnessType strictness,
                     ScopeType scope,
                     const QVariant& scopeRule,
                     bool isActive);

private:
    IgnoreList _ignoreList;
};
