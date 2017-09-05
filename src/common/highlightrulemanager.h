/***************************************************************************
 *   Copyright (C) 2005-2016 by the Quassel Project                        *
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

#ifndef HIGHLIGHTRULELISTMANAGER_H
#define HIGHLIGHTRULELISTMANAGER_H

#include <QString>
#include <QRegExp>

#include "message.h"
#include "syncableobject.h"

class HighlightRuleManager : public SyncableObject
{
    SYNCABLE_OBJECT
        Q_OBJECT
public:
    enum HighlightNickType {
        NoNick = 0x00,
        CurrentNick = 0x01,
        AllNicks = 0x02
    };

    inline HighlightRuleManager(QObject *parent = nullptr) : SyncableObject(parent) { setAllowClientUpdates(true); }
    HighlightRuleManager &operator=(const HighlightRuleManager &other);

    struct HighlightRule {
        QString name;
        bool isRegEx = false;
        bool isCaseSensitive = false;
        bool isEnabled = true;
        bool isInverse = false;
        QString chanName;
        HighlightRule() {}
        HighlightRule(const QString &name_, bool isRegEx_, bool isCaseSensitive_,
                       bool isEnabled_, bool isInverse_, const QString &chanName_)
            : name(name_), isRegEx(isRegEx_), isCaseSensitive(isCaseSensitive_), isEnabled(isEnabled_),
              isInverse(isInverse_), chanName(chanName_) {
        }
        bool operator!=(const HighlightRule &other)
        {
            return (name != other.name ||
                    isRegEx != other.isRegEx ||
                    isCaseSensitive != other.isCaseSensitive ||
                    isEnabled != other.isEnabled ||
                    isInverse != other.isInverse ||
                    chanName != other.chanName);
        }
    };
    typedef QList<HighlightRule> HighlightRuleList;

    int indexOf(const QString &rule) const;
    inline bool contains(const QString &rule) const { return indexOf(rule) != -1; }
    inline bool isEmpty() const { return _highlightRuleList.isEmpty(); }
    inline int count() const { return _highlightRuleList.count(); }
    inline void removeAt(int index) { _highlightRuleList.removeAt(index); }
    inline void clear() { _highlightRuleList.clear(); }
    inline HighlightRule &operator[](int i) { return _highlightRuleList[i]; }
    inline const HighlightRule &operator[](int i) const { return _highlightRuleList.at(i); }
    inline const HighlightRuleList &highlightRuleList() const { return _highlightRuleList; }

    inline HighlightNickType highlightNick() { return _highlightNick; }
    inline bool  nicksCaseSensitive() { return _nicksCaseSensitive; }

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
    virtual inline void requestRemoveHighlightRule(const QString &highlightRule) { REQUEST(ARG(highlightRule)) }
    virtual void removeHighlightRule(const QString &highlightRule);

    //! Request toggling of "isEnabled" flag of a given ignore rule.
    /** Use this method if you want to toggle the "isEnabled" flag of a single ignore rule
      * and get that synced with the core immediately.
      * \param highlightRule A valid ignore rule
      */
    virtual inline void requestToggleHighlightRule(const QString &highlightRule) { REQUEST(ARG(highlightRule)) }
    virtual void toggleHighlightRule(const QString &highlightRule);

    //! Request an HighlightRule to be added to the ignore list
    /** Items added to the list with this method, get immediately synced with the core
      * \param name The rule
      * \param isRegEx If the rule should be interpreted as a nickname, or a regex
      * \param isCaseSensitive If the rule should be interpreted as case-sensitive
      * \param isEnabled If the rule is active
      * @param chanName The channel in which the rule should apply
      */
    virtual inline void requestAddHighlightRule(const QString &name, bool isRegEx, bool isCaseSensitive, bool isEnabled,
                                                bool isInverse, const QString &chanName)
    {
        REQUEST(ARG(name), ARG(isRegEx), ARG(isCaseSensitive), ARG(isEnabled), ARG(isInverse), ARG(chanName))
    }


    virtual void addHighlightRule(const QString &name, bool isRegEx, bool isCaseSensitive,
                                  bool isEnabled, bool isInverse, const QString &chanName);

    virtual inline void requestSetHighlightNick(HighlightNickType highlightNick)
    {
        REQUEST(ARG(highlightNick))
    }
    inline void setHighlightNick(HighlightNickType highlightNick) { _highlightNick = highlightNick; }

    virtual inline void requestSetNicksCaseSensitive(bool nicksCaseSensitive)
    {
        REQUEST(ARG(nicksCaseSensitive))
    }
    inline void setNicksCaseSensitive(bool nicksCaseSensitive) { _nicksCaseSensitive = nicksCaseSensitive; }

protected:
    void setHighlightRuleList(const QList<HighlightRule> &HighlightRuleList) { _highlightRuleList = HighlightRuleList; }

    bool _match(const QString &msgContents, const QString &msgSender, Message::Type msgType, Message::Flags msgFlags, const QString &bufferName, const QString &currentNick, const QStringList identityNicks);

signals:
    void ruleAdded(QString name, bool isRegEx, bool isCaseSensitive, bool isEnabled, bool isInverse, QString chanName);

private:
    HighlightRuleList _highlightRuleList;
    HighlightNickType _highlightNick = HighlightNickType::CurrentNick;
    bool _nicksCaseSensitive = false;
};


#endif // HIGHLIGHTRULELISTMANAGER_H
