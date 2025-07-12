/***************************************************************************
 *   Copyright (C) 2005-2022 by the Quassel Project                        *
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

#include "tabcompleter.h"

#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QTextCursor>

#include "action.h"
#include "actioncollection.h"
#include "buffermodel.h"
#include "client.h"
#include "graphicalui.h"
#include "ircchannel.h"
#include "ircuser.h"
#include "multilineedit.h"
#include "network.h"
#include "networkmodel.h"
#include "uisettings.h"

const Network* TabCompleter::_currentNetwork;
BufferId TabCompleter::_currentBufferId;
QString TabCompleter::_currentBufferName;
TabCompleter::Type TabCompleter::_completionType;

TabCompleter::TabCompleter(MultiLineEdit* _lineEdit)
    : QObject(_lineEdit)
    , _lineEdit(_lineEdit)
    , _enabled(false)
    , _nickSuffix(": ")
    , _lastCompletionLength(0)
    , _completionStartPos(0)
{
    _lineEdit->installEventFilter(this);
    ActionCollection* coll = GraphicalUi::actionCollection("General");
    QAction* a = coll->addAction("TabCompletionKey",
                                 new Action(tr("Tab completion"), coll, this, &TabCompleter::onTabCompletionKey, QKeySequence(Qt::Key_Tab)));
    a->setEnabled(false);  // avoid catching the shortcut
}

void TabCompleter::onTabCompletionKey()
{
    // do nothing; we use the event filter instead
}

void TabCompleter::buildCompletionList()
{
    _completionMap.clear();
    _nextCompletion = _completionMap.begin();

    QModelIndex currentIndex = Client::bufferModel()->currentIndex();
    _currentBufferId = currentIndex.data(NetworkModel::BufferIdRole).value<BufferId>();
    if (!_currentBufferId.isValid())
        return;

    NetworkId networkId = currentIndex.data(NetworkModel::NetworkIdRole).value<NetworkId>();
    _currentBufferName = currentIndex.sibling(currentIndex.row(), 0).data().toString();

    _currentNetwork = Client::network(networkId);
    if (!_currentNetwork)
        return;

    QString textBeforeCursor = _lineEdit->text().left(_lineEdit->cursorPosition());
    QRegularExpression wordBoundary(R"(\s)");  // whitespace only
    int lastSpace = textBeforeCursor.lastIndexOf(wordBoundary);
    QString tabAbbrev = textBeforeCursor.mid(lastSpace + 1);
    
    // For matching nicks, create a simple prefix match regex
    QString regexPattern = "^" + QRegularExpression::escape(tabAbbrev);
    QRegularExpression regex(regexPattern, QRegularExpression::CaseInsensitiveOption);
    
    if (!regex.isValid()) {
        return;
    }

    if (tabAbbrev.startsWith('#')) {
        _completionType = ChannelTab;
        for (IrcChannel* ircChannel : *(_currentNetwork->ircChannels())) {
            if (regex.match(ircChannel->name()).hasMatch())
                _completionMap[ircChannel->name()] = ircChannel->name();
        }
    }
    else {
        _completionType = UserTab;
        switch (static_cast<BufferInfo::Type>(currentIndex.data(NetworkModel::BufferTypeRole).toInt())) {
        case BufferInfo::ChannelBuffer: {
            IrcChannel* channel = _currentNetwork->ircChannel(_currentBufferName);
            if (!channel)
                return;
            for (IrcUser* ircUser : channel->ircUsers()) {
                if (regex.match(ircUser->nick()).hasMatch())
                    _completionMap[ircUser->nick().toLower()] = ircUser->nick();
            }
        } break;
        case BufferInfo::QueryBuffer:
            if (regex.match(_currentBufferName).hasMatch())
                _completionMap[_currentBufferName.toLower()] = _currentBufferName;
            // fallthrough
        case BufferInfo::StatusBuffer:
            if (!_currentNetwork->myNick().isEmpty() && regex.match(_currentNetwork->myNick()).hasMatch())
                _completionMap[_currentNetwork->myNick().toLower()] = _currentNetwork->myNick();
            break;
        default:
            return;
        }
    }

    _nextCompletion = _completionMap.begin();
    _lastCompletionLength = tabAbbrev.length();
    _completionStartPos = lastSpace + 1;
}

void TabCompleter::complete()
{
    TabCompletionSettings s;
    _nickSuffix = s.completionSuffix();

    if (!_enabled) {
        buildCompletionList();
        _enabled = true;
    }

    if (_nextCompletion != _completionMap.end()) {
        QTextCursor cursor = _lineEdit->textCursor();
        
        cursor.beginEditBlock();
        cursor.clearSelection();
        cursor.setPosition(_completionStartPos);
        cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, _lastCompletionLength);
        cursor.removeSelectedText();
        cursor.insertText(*_nextCompletion);
        cursor.endEditBlock();
        
        _lineEdit->setTextCursor(cursor);

        // Check if we need to add suffix
        bool atEndOfLine = (_lineEdit->cursorPosition() == _lineEdit->text().length());
        
        _lastCompletionLength = _nextCompletion->length();
        _nextCompletion++;

        if (_completionType == UserTab && atEndOfLine) {
            _lineEdit->insert(_nickSuffix);
            _lastCompletionLength += _nickSuffix.length();
        }
        else if (s.addSpaceMidSentence()) {
            _lineEdit->addCompletionSpace();
            _lastCompletionLength++;
        }
    }
    else {
        if (!_completionMap.isEmpty()) {
            _nextCompletion = _completionMap.begin();
            complete();
        }
    }
}

void TabCompleter::reset()
{
    _enabled = false;
}

bool TabCompleter::eventFilter(QObject* obj, QEvent* event)
{
    if (obj != _lineEdit || event->type() != QEvent::KeyPress)
        return QObject::eventFilter(obj, event);

    auto* keyEvent = static_cast<QKeyEvent*>(event);

    QAction* tabCompletionAction = GraphicalUi::actionCollection("General")->action("TabCompletionKey");
    if (keyEvent->key() == Qt::Key_Tab) {
        complete();
    } else if (tabCompletionAction && keyEvent->keyCombination() == tabCompletionAction->shortcut()[0]) {
        complete();
    } else {
        reset();
    }

    return false;
}

bool TabCompleter::CompletionKey::operator<(const CompletionKey& other) const
{
    switch (_completionType) {
    case UserTab: {
        IrcUser* thisUser = _currentNetwork->ircUser(this->contents);
        if (thisUser && _currentNetwork->isMe(thisUser))
            return false;

        IrcUser* thatUser = _currentNetwork->ircUser(other.contents);
        if (thatUser && _currentNetwork->isMe(thatUser))
            return true;

        if (!thisUser || !thatUser)
            return QString::localeAwareCompare(this->contents, other.contents) < 0;

        QDateTime thisSpokenTo = thisUser->lastSpokenTo(_currentBufferId);
        QDateTime thatSpokenTo = thatUser->lastSpokenTo(_currentBufferId);

        if (thisSpokenTo.isValid() || thatSpokenTo.isValid())
            return thisSpokenTo > thatSpokenTo;

        QDateTime thisTime = thisUser->lastChannelActivity(_currentBufferId);
        QDateTime thatTime = thatUser->lastChannelActivity(_currentBufferId);

        if (thisTime.isValid() || thatTime.isValid())
            return thisTime > thatTime;
    } break;
    case ChannelTab:
        if (QString::compare(_currentBufferName, this->contents, Qt::CaseInsensitive) == 0)
            return true;

        if (QString::compare(_currentBufferName, other.contents, Qt::CaseInsensitive) == 0)
            return false;
        break;
    default:
        break;
    }

    return QString::localeAwareCompare(this->contents, other.contents) < 0;
}
