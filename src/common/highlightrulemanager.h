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

#include <utility>

#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include "expressionmatch.h"
#include "message.h"
#include "nickhighlightmatcher.h"
#include "syncableobject.h"

class HighlightRuleManager : public SyncableObject
{
    SYNCABLE_OBJECT
    Q_OBJECT

    Q_PROPERTY(int highlightNick READ highlightNick WRITE setHighlightNick)
    Q_PROPERTY(bool nicksCaseSensitive READ nicksCaseSensitive WRITE setNicksCaseSensitive)

public:
    enum HighlightNickType {
        NoNick = 0x00,
        CurrentNick = 0x01,
        AllNicks = 0x02
    };

    inline HighlightRuleManager(QObject *parent = nullptr) : SyncableObject(parent) { setAllowClientUpdates(true); }
    HighlightRuleManager &operator=(const HighlightRuleManager &other);

    /**
     * Individual highlight rule
     */
    class HighlightRule
    {
    public:
        /**
         * Construct an empty highlight rule
         */
        HighlightRule() {}

        /**
         * Construct a highlight rule with the given parameters
         *
         * @param id               Integer ID of the rule
         * @param contents         String representing a message contents expression to match
         * @param isRegEx          True if regular expression, otherwise false
         * @param isCaseSensitive  True if case sensitive, otherwise false
         * @param isEnabled        True if enabled, otherwise false
         * @param isInverse        True if rule is treated as highlight ignore, otherwise false
         * @param sender           String representing a message sender expression to match
         * @param chanName         String representing a channel name expression to match
         */
        HighlightRule(int id, QString contents, bool isRegEx, bool isCaseSensitive, bool isEnabled,
                      bool isInverse, QString sender, QString chanName)
            : _id(id), _contents(contents), _isRegEx(isRegEx), _isCaseSensitive(isCaseSensitive),
              _isEnabled(isEnabled), _isInverse(isInverse), _sender(sender), _chanName(chanName)
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
         * Gets the unique ID of this rule
         *
         * @return Integer ID of the rule
         */
        inline int id() const {
            return _id;
        }
        /**
         * Sets the ID of this rule
         *
         * CAUTION: IDs should be kept unique!
         *
         * @param id Integer ID of the rule
         */
        inline void setId(int id) {
            _id = id;
        }

        /**
         * Gets the message contents this rule matches
         *
         * NOTE: Use HighlightRule::contentsMatcher() for performing matches
         *
         * @return String representing a phrase or expression to match
         */
        inline QString contents() const {
            return _contents;
        }
        /**
         * Sets the message contents this rule matches
         *
         * @param contents String representing a phrase or expression to match
         */
        inline void setContents(const QString &contents) {
            _contents = contents;
            _cacheInvalid = true;
        }

        /**
         * Gets if this is a regular expression rule
         *
         * @return True if regular expression, otherwise false
         */
        inline bool isRegEx() const {
            return _isRegEx;
        }
        /**
         * Sets if this rule is a regular expression rule
         *
         * @param isRegEx True if regular expression, otherwise false
         */
        inline void setIsRegEx(bool isRegEx) {
            _isRegEx = isRegEx;
            _cacheInvalid = true;
        }

        /**
         * Gets if this rule is case sensitive
         *
         * @return True if case sensitive, otherwise false
         */
        inline bool isCaseSensitive() const {
            return _isCaseSensitive;
        }
        /**
         * Sets if this rule is case sensitive
         *
         * @param isCaseSensitive True if case sensitive, otherwise false
         */
        inline void setIsCaseSensitive(bool isCaseSensitive) {
            _isCaseSensitive = isCaseSensitive;
            _cacheInvalid = true;
        }

        /**
         * Gets if this rule is enabled and active
         *
         * @return True if enabled, otherwise false
         */
        inline bool isEnabled() const {
            return _isEnabled;
        }
        /**
         * Sets if this rule is enabled and active
         *
         * @param isEnabled True if enabled, otherwise false
         */
        inline void setIsEnabled(bool isEnabled) {
            _isEnabled = isEnabled;
        }

        /**
         * Gets if this rule is a highlight ignore rule
         *
         * @return True if rule is treated as highlight ignore, otherwise false
         */
        inline bool isInverse() const {
            return _isInverse;
        }
        /**
         * Sets if this rule is a highlight ignore rule
         *
         * @param isInverse True if rule is treated as highlight ignore, otherwise false
         */
        inline void setIsInverse(bool isInverse) {
            _isInverse = isInverse;
        }

        /**
         * Gets the message sender this rule matches
         *
         * NOTE: Use HighlightRule::senderMatcher() for performing matches
         *
         * @return String representing a phrase or expression to match
         */
        inline QString sender() const { return _sender; }
        /**
         * Sets the message sender this rule matches
         *
         * @param sender String representing a phrase or expression to match
         */
        inline void setSender(const QString &sender) {
            _sender = sender;
            _cacheInvalid = true;
        }

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
        inline void setChanName(const QString &chanName) {
            _chanName = chanName;
            _cacheInvalid = true;
        }

        /**
         * Gets the expression matcher for the message contents, caching if needed
         *
         * @return Expression matcher to compare with message contents
         */
        inline ExpressionMatch contentsMatcher() const {
            if (_cacheInvalid) {
                determineExpressions();
            }
            return _contentsMatch;
        }

        /**
         * Gets the expression matcher for the message sender, caching if needed
         *
         * @return Expression matcher to compare with message sender
         */
        inline ExpressionMatch senderMatcher() const {
            if (_cacheInvalid) {
                determineExpressions();
            }
            return _senderMatch;
        }

        /**
         * Gets the expression matcher for the channel name, caching if needed
         *
         * @return Expression matcher to compare with channel name
         */
        inline ExpressionMatch chanNameMatcher() const {
            if (_cacheInvalid) {
                determineExpressions();
            }
            return _chanNameMatch;
        }

        bool operator!=(const HighlightRule &other) const;

    private:
        /**
         * Update internal cache of expression matching if needed
         */
        void determineExpressions() const;

        int _id = -1;
        QString _contents = {};
        bool _isRegEx = false;
        bool _isCaseSensitive = false;
        bool _isEnabled = true;
        bool _isInverse = false;
        QString _sender = {};
        QString _chanName = {};

        // These represent internal cache and should be safe to mutate in 'const' functions
        // See https://stackoverflow.com/questions/3141087/what-is-meant-with-const-at-end-of-function-declaration
        mutable bool _cacheInvalid = true;           ///< If true, match cache needs redone
        mutable ExpressionMatch _contentsMatch = {}; ///< Expression match cache for message content
        mutable ExpressionMatch _senderMatch = {};   ///< Expression match cache for sender
        mutable ExpressionMatch _chanNameMatch = {}; ///< Expression match cache for channel name
    };

    using HighlightRuleList = QList<HighlightRule>;

    int indexOf(int rule) const;
    inline bool contains(int rule) const { return indexOf(rule) != -1; }
    inline bool isEmpty() const { return _highlightRuleList.isEmpty(); }
    inline int count() const { return _highlightRuleList.count(); }
    inline void removeAt(int index) { _highlightRuleList.removeAt(index); }
    inline void clear() { _highlightRuleList.clear(); }
    inline HighlightRule &operator[](int i) { return _highlightRuleList[i]; }
    inline const HighlightRule &operator[](int i) const { return _highlightRuleList.at(i); }
    inline const HighlightRuleList &highlightRuleList() const { return _highlightRuleList; }

    int nextId();

    inline int highlightNick() { return _highlightNick; }
    inline bool nicksCaseSensitive() { return _nicksCaseSensitive; }

    //! Check if a message matches the HighlightRule
    /** This method checks if a message matches the users highlight rules.
      * \param msg The Message that should be checked
      */
    bool match(const Message &msg, const QString &currentNick, const QStringList &identityNicks);

public slots:
    virtual QVariantMap initHighlightRuleList() const;
    virtual void initSetHighlightRuleList(const QVariantMap &HighlightRuleList);

    //! Request removal of an ignore rule based on the rule itself.
    /** Use this method if you want to remove a single ignore rule
      * and get that synced with the core immediately.
      * \param highlightRule A valid ignore rule
      */
    virtual inline void requestRemoveHighlightRule(int highlightRule) { REQUEST(ARG(highlightRule)) }
    virtual void removeHighlightRule(int highlightRule);

    //! Request toggling of "isEnabled" flag of a given ignore rule.
    /** Use this method if you want to toggle the "isEnabled" flag of a single ignore rule
      * and get that synced with the core immediately.
      * \param highlightRule A valid ignore rule
      */
    virtual inline void requestToggleHighlightRule(int highlightRule) { REQUEST(ARG(highlightRule)) }
    virtual void toggleHighlightRule(int highlightRule);

    //! Request an HighlightRule to be added to the ignore list
    /** Items added to the list with this method, get immediately synced with the core
      * \param name The rule
      * \param isRegEx If the rule should be interpreted as a nickname, or a regex
      * \param isCaseSensitive If the rule should be interpreted as case-sensitive
      * \param isEnabled If the rule is active
      * @param chanName The channel in which the rule should apply
      */
    virtual inline void requestAddHighlightRule(int id, const QString &name, bool isRegEx, bool isCaseSensitive, bool isEnabled,
                                                bool isInverse, const QString &sender, const QString &chanName)
    {
        REQUEST(ARG(id), ARG(name), ARG(isRegEx), ARG(isCaseSensitive), ARG(isEnabled), ARG(isInverse), ARG(sender),
                ARG(chanName))
    }


    virtual void addHighlightRule(int id, const QString &name, bool isRegEx, bool isCaseSensitive, bool isEnabled,
                                  bool isInverse, const QString &sender, const QString &chanName);

    virtual inline void requestSetHighlightNick(int highlightNick)
    {
        REQUEST(ARG(highlightNick))
    }

    inline void setHighlightNick(int highlightNick) {
        _highlightNick = static_cast<HighlightNickType>(highlightNick);
        // Convert from HighlightRuleManager::HighlightNickType to
        // NickHighlightMatcher::HighlightNickType
        _nickMatcher.setHighlightMode(
                    static_cast<NickHighlightMatcher::HighlightNickType>(_highlightNick));
    }

    virtual inline void requestSetNicksCaseSensitive(bool nicksCaseSensitive)
    {
        REQUEST(ARG(nicksCaseSensitive))
    }

    inline void setNicksCaseSensitive(bool nicksCaseSensitive) {
        _nicksCaseSensitive = nicksCaseSensitive;
        // Update nickname matcher, too
        _nickMatcher.setCaseSensitive(nicksCaseSensitive);
    }

    /**
     * Network removed from system
     *
     * Handles cleaning up cache from stale networks.
     *
     * @param id Network ID of removed network
     */
    inline void networkRemoved(NetworkId id) {
        // Clean up nickname matching cache
        _nickMatcher.removeNetwork(id);
    }

protected:
    void setHighlightRuleList(const QList<HighlightRule> &HighlightRuleList) { _highlightRuleList = HighlightRuleList; }

    bool match(const NetworkId &netId,
               const QString &msgContents,
               const QString &msgSender,
               Message::Type msgType,
               Message::Flags msgFlags,
               const QString &bufferName,
               const QString &currentNick,
               const QStringList &identityNicks);

signals:
    void ruleAdded(QString name, bool isRegEx, bool isCaseSensitive, bool isEnabled, bool isInverse, QString sender, QString chanName);

private:
    HighlightRuleList _highlightRuleList = {}; ///< Custom highlight rule list
    NickHighlightMatcher _nickMatcher = {};    ///< Nickname highlight matcher

    /// Nickname highlighting mode
    HighlightNickType _highlightNick = HighlightNickType::CurrentNick;
    bool _nicksCaseSensitive = false; ///< If true, match nicknames with exact case
};
