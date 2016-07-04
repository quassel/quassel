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

#ifndef IGNORELISTMANAGER_H
#define IGNORELISTMANAGER_H

#include <QString>
#include <QRegularExpression>

#include "message.h"
#include "syncableobject.h"

class IgnoreListManager : public SyncableObject
{
    SYNCABLE_OBJECT
        Q_OBJECT
public:
    inline IgnoreListManager(QObject *parent = 0) : SyncableObject(parent) { setAllowClientUpdates(true); }
    IgnoreListManager &operator=(const IgnoreListManager &other);

    enum IgnoreType {
        SenderIgnore,
        MessageIgnore,
        CtcpIgnore
    };

    enum StrictnessType {
        UnmatchedStrictness = 0,
        SoftStrictness = 1,
        HardStrictness = 2
    };

    enum ScopeType {
        GlobalScope,
        NetworkScope,
        ChannelScope,
    };

    static QString convertFromWildcard(const QString originalRule) {
        QString pattern(QRegularExpression::escape(originalRule));
        pattern.replace("\\?", ".");
        pattern.replace("\\*", ".*");
        pattern = '^' + pattern + '$';
        return pattern;
    }

    struct IgnoreListItem {
        IgnoreType type;
        QString ignoreRule;
        bool isRegEx;
        StrictnessType strictness;
        ScopeType scope;
        QString scopeRule;
        QRegularExpression scopeRegex;
        bool isActive;
        QRegularExpression regEx;
        QStringList ctcpTypes;
        IgnoreListItem() {}
        IgnoreListItem(IgnoreType type_, const QString &ignoreRule_, bool isRegEx_, StrictnessType strictness_,
            ScopeType scope_, const QString &scopeRule_, bool isActive_)
            : type(type_), ignoreRule(ignoreRule_), isRegEx(isRegEx_), strictness(strictness_), scope(scope_), isActive(isActive_) {

            if (ignoreRule.isEmpty()) {
                qWarning() << "Handed an empty ignore rule";
                return;
            }


            QString pattern(ignoreRule);

            if (type == CtcpIgnore) {
                QStringList split(pattern.split(QRegularExpression("\\s+"), QString::SkipEmptyParts));
                pattern = split.takeFirst();
                ctcpTypes = split;
            }

            if (!isRegEx) {
                pattern = convertFromWildcard(pattern);
            }

            regEx.setPattern(pattern);
            regEx.setPatternOptions(QRegularExpression::CaseInsensitiveOption);

            if (!regEx.isValid()) {
                qWarning() << "Ignore rule" << ignoreRule << "is invalid!";
            } else {
#if QT_VERSION >= 0x050400
                regEx.optimize();
#endif
            }

            // Set up scope regex
            pattern = convertFromWildcard(scopeRule);
            pattern.replace(';', '|');
            scopeRegex = QRegularExpression(scopeRule_);
            scopeRegex.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
            if (!scopeRegex.isValid()) {
                qWarning() << "Scope rule" << scopeRule_ << "is invalid!";
            } else {
#if QT_VERSION >= 0x050400
                scopeRegex.optimize();
#endif
            }
        }
        bool operator!=(const IgnoreListItem &other)
        {
            return (type != other.type ||
                    ignoreRule != other.ignoreRule ||
                    isRegEx != other.isRegEx ||
                    strictness != other.strictness ||
                    scope != other.scope ||
                    scopeRule != other.scopeRule ||
                    isActive != other.isActive);
        }
    };
    typedef QList<IgnoreListItem> IgnoreList;

    int indexOf(const QString &ignore) const;
    inline bool contains(const QString &ignore) const { return indexOf(ignore) != -1; }
    inline bool isEmpty() const { return _ignoreList.isEmpty(); }
    inline int count() const { return _ignoreList.count(); }
    inline void removeAt(int index) { _ignoreList.removeAt(index); }
    inline IgnoreListItem &operator[](int i) { return _ignoreList[i]; }
    inline const IgnoreListItem &operator[](int i) const { return _ignoreList.at(i); }
    inline const IgnoreList &ignoreList() const { return _ignoreList; }

    //! Check if a message matches the IgnoreRule
    /** This method checks if a message matches the users ignorelist.
      * \param msg The Message that should be checked
      * \param network The networkname the message belongs to
      * \return UnmatchedStrictness, HardStrictness or SoftStrictness representing the match type
      */
    inline StrictnessType match(const Message &msg, const QString &network = QString()) { return _match(msg.contents(), msg.sender(), msg.type(), network, msg.bufferInfo().bufferName()); }

    bool ctcpMatch(const QString sender, const QString &network, const QString &type = QString());

//  virtual void addIgnoreListItem(const IgnoreListItem &item);

public slots:
    virtual QVariantMap initIgnoreList() const;
    virtual void initSetIgnoreList(const QVariantMap &ignoreList);

    //! Request removal of an ignore rule based on the rule itself.
    /** Use this method if you want to remove a single ignore rule
      * and get that synced with the core immediately.
      * \param ignoreRule A valid ignore rule
      */
    virtual inline void requestRemoveIgnoreListItem(const QString &ignoreRule) { REQUEST(ARG(ignoreRule)) }
    virtual void removeIgnoreListItem(const QString &ignoreRule);

    //! Request toggling of "isActive" flag of a given ignore rule.
    /** Use this method if you want to toggle the "isActive" flag of a single ignore rule
      * and get that synced with the core immediately.
      * \param ignoreRule A valid ignore rule
      */
    virtual inline void requestToggleIgnoreRule(const QString &ignoreRule) { REQUEST(ARG(ignoreRule)) }
    virtual void toggleIgnoreRule(const QString &ignoreRule);

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
    virtual inline void requestAddIgnoreListItem(int type, const QString &ignoreRule, bool isRegEx, int strictness,
        int scope, const QString &scopeRule, bool isActive)
    {
        REQUEST(ARG(type), ARG(ignoreRule), ARG(isRegEx), ARG(strictness), ARG(scope), ARG(scopeRule), ARG(isActive))
    }


    virtual void addIgnoreListItem(int type, const QString &ignoreRule, bool isRegEx, int strictness,
        int scope, const QString &scopeRule, bool isActive);

protected:
    void setIgnoreList(const QList<IgnoreListItem> &ignoreList) { _ignoreList = ignoreList; }
    bool scopeMatch(const QRegularExpression &scopeRegex, const QString &string) const;

    StrictnessType _match(const QString &msgContents, const QString &msgSender, Message::Type msgType, const QString &network, const QString &bufferName);

signals:
    void ignoreAdded(IgnoreType type, const QString &ignoreRule, bool isRegex, StrictnessType strictness, ScopeType scope, const QVariant &scopeRule, bool isActive);

private:
    IgnoreList _ignoreList;
};


#endif // IGNORELISTMANAGER_H
