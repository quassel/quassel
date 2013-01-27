/***************************************************************************
 *   Copyright (C) 2005-2013 by the Quassel Project                        *
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

#include "netsplit.h"
#include "network.h"
#include "util.h"

#include <QRegExp>

Netsplit::Netsplit(Network *network, QObject *parent)
    : QObject(parent),
    _network(network), _quitMsg(""), _sentQuit(false), _joinCounter(0), _quitCounter(0)
{
    _discardTimer.setSingleShot(true);
    _joinTimer.setSingleShot(true);
    _quitTimer.setSingleShot(true);

    connect(&_discardTimer, SIGNAL(timeout()), this, SIGNAL(finished()));

    connect(&_joinTimer, SIGNAL(timeout()), this, SLOT(joinTimeout()));
    connect(&_quitTimer, SIGNAL(timeout()), this, SLOT(quitTimeout()));

    // wait for a maximum of 1 hour until we discard the netsplit
    _discardTimer.start(3600000);
}


void Netsplit::userQuit(const QString &sender, const QStringList &channels, const QString &msg)
{
    if (_quitMsg.isEmpty())
        _quitMsg = msg;
    foreach(QString channel, channels) {
        _quits[channel].append(sender);
    }
    _quitCounter++;
    // now let's wait 10s to finish the netsplit-quit
    _quitTimer.start(10000);
}


bool Netsplit::userJoined(const QString &sender, const QString &channel)
{
    if (!_quits.contains(channel))
        return false;

    QStringList &users = _quits[channel];

    QStringList::iterator userIter;
    const QString senderNick = nickFromMask(sender);
    for (userIter = users.begin(); userIter != users.end(); ++userIter) {
        if (nickFromMask(*userIter) == senderNick)
            break;
    }
    if (userIter == users.end())
        return false;

    _joins[channel].first.append(*userIter);
    _joins[channel].second.append(QString());

    users.erase(userIter);

    if (users.empty())
        _quits.remove(channel);

    _joinCounter++;

    if (_quits.empty()) // all users joined already - no need to wait
        _joinTimer.start(0);
    else // wait 30s to finish the netsplit-join
        _joinTimer.start(30000);

    return true;
}


bool Netsplit::userAlreadyJoined(const QString &sender, const QString &channel)
{
    if (_joins.value(channel).first.contains(sender))
        return true;
    return false;
}


void Netsplit::addMode(const QString &sender, const QString &channel, const QString &mode)
{
    if (!_joins.contains(channel))
        return;
    int idx = _joins.value(channel).first.indexOf(sender);
    if (idx == -1)
        return;
    _joins[channel].second[idx].append(mode);
}


bool Netsplit::isNetsplit(const QString &quitMessage)
{
    // check if we find some common chars that disqualify the netsplit as such
    if (quitMessage.contains(':') || quitMessage.contains('/'))
        return false;

    // now test if message consists only of two dns names as the RFC requests
    // but also allow the commonly used "*.net *.split"
    QRegExp hostRx("^(?:[\\w\\d-.]+|\\*)\\.[\\w\\d-]+\\s(?:[\\w\\d-.]+|\\*)\\.[\\w\\d-]+$");
    if (hostRx.exactMatch(quitMessage))
        return true;

    return false;
}


void Netsplit::joinTimeout()
{
    if (!_sentQuit) {
        _quitTimer.stop();
        quitTimeout();
    }

    QHash<QString, QPair<QStringList, QStringList> >::iterator it;

    /*
      Try to catch server jumpers.
      If we have too few joins for a netsplit-quit,
      we assume that the users manually changed servers and join them
      without ending the netsplit.
      A netsplit is assumed over only if at least 1/3 of all quits had their corresponding
      join again.
    */
    if (_joinCounter < _quitCounter/3) {
        for (it = _joins.begin(); it != _joins.end(); ++it)
            emit earlyJoin(network(), it.key(), it.value().first, it.value().second);

        // we don't care about those anymore
        _joins.clear();

        // restart the timer with 5min timeout
        // This might happen a few times if netsplit lasts longer.
        // As soon as another user joins, the timer is set to a shorter timeout again.
        _joinTimer.start(300000);
        return;
    }

    // send netsplitJoin for every recorded channel
    for (it = _joins.begin(); it != _joins.end(); ++it)
        emit netsplitJoin(network(), it.key(), it.value().first, it.value().second, _quitMsg);
    _joins.clear();
    _discardTimer.stop();
    emit finished();
}


void Netsplit::quitTimeout()
{
    // send netsplitQuit for every recorded channel
    QHash<QString, QStringList>::iterator channelIter;
    for (channelIter = _quits.begin(); channelIter != _quits.end(); ++channelIter) {
        QStringList usersToSend;

        foreach(QString user, channelIter.value()) {
            if (!_quitsWithMessageSent.value(channelIter.key()).contains(user)) {
                usersToSend << user;
                _quitsWithMessageSent[channelIter.key()].append(user);
            }
        }
        // not yet sure how that could happen, but never send empty netsplit-quits
        // anyway.
        if (!usersToSend.isEmpty())
            emit netsplitQuit(network(), channelIter.key(), usersToSend, _quitMsg);
    }
    _sentQuit = true;
}
