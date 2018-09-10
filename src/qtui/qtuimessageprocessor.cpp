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

#include "qtuimessageprocessor.h"

#include "client.h"
#include "clientsettings.h"
#include "identity.h"
#include "messagemodel.h"
#include "network.h"

QtUiMessageProcessor::QtUiMessageProcessor(QObject *parent)
    : AbstractMessageProcessor(parent),
    _processing(false),
    _processMode(TimerBased)
{
    NotificationSettings notificationSettings;
    _nicksCaseSensitive = notificationSettings.nicksCaseSensitive();
    _nickMatcher.setCaseSensitive(_nicksCaseSensitive);
    _highlightNick = notificationSettings.highlightNick();
    _nickMatcher.setHighlightMode(
                static_cast<NickHighlightMatcher::HighlightNickType>(_highlightNick));
    highlightListChanged(notificationSettings.highlightList());
    notificationSettings.notify("Highlights/NicksCaseSensitive", this, SLOT(nicksCaseSensitiveChanged(const QVariant &)));
    notificationSettings.notify("Highlights/CustomList", this, SLOT(highlightListChanged(const QVariant &)));
    notificationSettings.notify("Highlights/HighlightNick", this, SLOT(highlightNickChanged(const QVariant &)));

    _processTimer.setInterval(0);
    connect(&_processTimer, &QTimer::timeout, this, &QtUiMessageProcessor::processNextMessage);
}


void QtUiMessageProcessor::reset()
{
    if (processMode() == TimerBased) {
        if (_processTimer.isActive()) _processTimer.stop();
        _processing = false;
        _currentBatch.clear();
        _processQueue.clear();
    }
}


void QtUiMessageProcessor::process(Message &msg)
{
    checkForHighlight(msg);
    preProcess(msg);
    Client::messageModel()->insertMessage(msg);
}


void QtUiMessageProcessor::process(QList<Message> &msgs)
{
    QList<Message>::iterator msgIter = msgs.begin();
    QList<Message>::iterator msgIterEnd = msgs.end();
    while (msgIter != msgIterEnd) {
        checkForHighlight(*msgIter);
        preProcess(*msgIter);
        ++msgIter;
    }
    Client::messageModel()->insertMessages(msgs);
    return;

    if (msgs.isEmpty()) return;
    _processQueue.append(msgs);
    if (!isProcessing())
        startProcessing();
}


void QtUiMessageProcessor::startProcessing()
{
    if (processMode() == TimerBased) {
        if (_currentBatch.isEmpty() && _processQueue.isEmpty())
            return;
        _processing = true;
        if (!_processTimer.isActive())
            _processTimer.start();
    }
}


void QtUiMessageProcessor::processNextMessage()
{
    if (_currentBatch.isEmpty()) {
        if (_processQueue.isEmpty()) {
            _processTimer.stop();
            _processing = false;
            return;
        }
        _currentBatch = _processQueue.takeFirst();
    }
    Message msg = _currentBatch.takeFirst();
    process(msg);
}


void QtUiMessageProcessor::checkForHighlight(Message &msg)
{
    if (!((msg.type() & (Message::Plain | Message::Notice | Message::Action)) && !(msg.flags() & Message::Self)))
        return;

    // Cached per network
    const NetworkId &netId = msg.bufferInfo().networkId();
    const Network *net = Client::network(netId);

    if (net && !net->myNick().isEmpty()) {
        // Get current nick
        QString currentNick = net->myNick();
        // Get identity nicks
        QStringList identityNicks = {};
        const Identity *myIdentity = Client::identity(net->identity());
        if (myIdentity) {
            identityNicks = myIdentity->nicks();
        }

        // Get buffer name, message contents
        QString bufferName = msg.bufferInfo().bufferName();
        QString msgContents = msg.contents();
        bool matches = false;

        for (int i = 0; i < _highlightRuleList.count(); i++) {
            auto &rule = _highlightRuleList.at(i);
            if (!rule.isEnabled())
                continue;

            // Skip if channel name doesn't match and channel rule is not empty
            //
            // Match succeeds if...
            //   Channel name matches a defined rule
            //   Defined rule is empty
            // And take the inverse of the above
            if (!rule.chanNameMatcher().match(bufferName, true)) {
                // A channel name rule is specified and does NOT match the current buffer name, skip
                // this rule
                continue;
            }

            // Check message according to specified rule, allowing empty rules to match
            bool contentsMatch = rule.contentsMatcher().match(stripFormatCodes(msgContents), true);

            // Support for sender matching can be added here

            if (contentsMatch) {
                // Support for inverse rules can be added here
                matches = true;
            }
        }

        if (matches) {
            msg.setFlags(msg.flags() | Message::Highlight);
            return;
        }

        // Check nicknames
        if (_highlightNick != HighlightNickType::NoNick && !currentNick.isEmpty()) {
            // Nickname matching allowed and current nickname is known
            // Run the nickname matcher on the unformatted string
            if (_nickMatcher.match(stripFormatCodes(msgContents), netId, currentNick,
                                   identityNicks)) {
                msg.setFlags(msg.flags() | Message::Highlight);
                return;
            }
        }
    }
}


void QtUiMessageProcessor::nicksCaseSensitiveChanged(const QVariant &variant)
{
    _nicksCaseSensitive = variant.toBool();
    // Update nickname matcher, too
    _nickMatcher.setCaseSensitive(_nicksCaseSensitive);
}


void QtUiMessageProcessor::highlightListChanged(const QVariant &variant)
{
    QVariantList varList = variant.toList();

    _highlightRuleList.clear();
    QVariantList::const_iterator iter = varList.constBegin();
    while (iter != varList.constEnd()) {
        QVariantMap rule = iter->toMap();
        _highlightRuleList << LegacyHighlightRule(rule["Name"].toString(),
                rule["RegEx"].toBool(),
                rule["CS"].toBool(),
                rule["Enable"].toBool(),
                rule["Channel"].toString());
        ++iter;
    }
}


void QtUiMessageProcessor::highlightNickChanged(const QVariant &variant)
{
    _highlightNick = (HighlightNickType)variant.toInt();
    // Convert from QtUiMessageProcessor::HighlightNickType (which is from NotificationSettings) to
    // NickHighlightMatcher::HighlightNickType
    _nickMatcher.setHighlightMode(
                static_cast<NickHighlightMatcher::HighlightNickType>(_highlightNick));
}


/**************************************************************************
 * LegacyHighlightRule
 *************************************************************************/
bool QtUiMessageProcessor::LegacyHighlightRule::operator!=(const LegacyHighlightRule &other) const
{
    return (_contents != other._contents ||
            _isRegEx != other._isRegEx ||
            _isCaseSensitive != other._isCaseSensitive ||
            _isEnabled != other._isEnabled ||
            _chanName != other._chanName);
    // Don't compare ExpressionMatch objects as they are created as needed from the above
}


void QtUiMessageProcessor::LegacyHighlightRule::determineExpressions() const
{
    // Don't update if not needed
    if (!_cacheInvalid) {
        return;
    }

    // Set up matching rules
    // Message is either phrase or regex
    ExpressionMatch::MatchMode contentsMode =
            _isRegEx ? ExpressionMatch::MatchMode::MatchRegEx :
                       ExpressionMatch::MatchMode::MatchPhrase;
    // Sender (when added) and channel are either multiple wildcard entries or regex
    ExpressionMatch::MatchMode scopeMode =
            _isRegEx ? ExpressionMatch::MatchMode::MatchRegEx :
                       ExpressionMatch::MatchMode::MatchMultiWildcard;

    _contentsMatch = ExpressionMatch(_contents, contentsMode, _isCaseSensitive);
    _chanNameMatch = ExpressionMatch(_chanName, scopeMode, _isCaseSensitive);

    _cacheInvalid = false;
}
