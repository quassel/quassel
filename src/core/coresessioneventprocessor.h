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

#ifndef CORESESSIONEVENTPROCESSOR_H
#define CORESESSIONEVENTPROCESSOR_H

#include "basichandler.h"
#include "corenetwork.h"
#include "networkevent.h"

class CoreSession;
class CtcpEvent;
class IrcEvent;
class IrcEventNumeric;
class Netsplit;

#ifdef HAVE_QCA2
class KeyEvent;
#endif

class CoreSessionEventProcessor : public BasicHandler
{
    Q_OBJECT

public:
    CoreSessionEventProcessor(CoreSession *session);

    inline CoreSession *coreSession() const { return _coreSession; }

    Q_INVOKABLE void processIrcEventNumeric(IrcEventNumeric *event);

    Q_INVOKABLE void processIrcEventAuthenticate(IrcEvent *event); // SASL auth
    Q_INVOKABLE void processIrcEventCap(IrcEvent *event);          // CAP framework
    Q_INVOKABLE void processIrcEventInvite(IrcEvent *event);
    Q_INVOKABLE void processIrcEventJoin(IrcEvent *event);
    Q_INVOKABLE void lateProcessIrcEventKick(IrcEvent *event);
    Q_INVOKABLE void processIrcEventMode(IrcEvent *event);
    Q_INVOKABLE void lateProcessIrcEventNick(IrcEvent *event);
    Q_INVOKABLE void lateProcessIrcEventPart(IrcEvent *event);
    Q_INVOKABLE void processIrcEventPing(IrcEvent *event);
    Q_INVOKABLE void processIrcEventPong(IrcEvent *event);
    Q_INVOKABLE void processIrcEventQuit(IrcEvent *event);
    Q_INVOKABLE void lateProcessIrcEventQuit(IrcEvent *event);
    Q_INVOKABLE void processIrcEventTopic(IrcEvent *event);
#ifdef HAVE_QCA2
    Q_INVOKABLE void processKeyEvent(KeyEvent *event);
#endif

    Q_INVOKABLE void processIrcEvent001(IrcEvent *event);          // RPL_WELCOME
    Q_INVOKABLE void processIrcEvent005(IrcEvent *event);          // RPL_ISUPPORT
    Q_INVOKABLE void processIrcEvent221(IrcEvent *event);          // RPL_UMODEIS
    Q_INVOKABLE void processIrcEvent250(IrcEvent *event);          // RPL_STATSCONN
    Q_INVOKABLE void processIrcEvent265(IrcEvent *event);          // RPL_LOCALUSERS
    Q_INVOKABLE void processIrcEvent266(IrcEvent *event);          // RPL_GLOBALUSERS
    Q_INVOKABLE void processIrcEvent301(IrcEvent *event);          // RPL_AWAY
    Q_INVOKABLE void processIrcEvent305(IrcEvent *event);          // RPL_UNAWAY
    Q_INVOKABLE void processIrcEvent306(IrcEvent *event);          // RPL_NOWAWAY
    Q_INVOKABLE void processIrcEvent307(IrcEvent *event);          // RPL_WHOISSERVICE
    Q_INVOKABLE void processIrcEvent310(IrcEvent *event);          // RPL_SUSERHOST
    Q_INVOKABLE void processIrcEvent311(IrcEvent *event);          // RPL_WHOISUSER
    Q_INVOKABLE void processIrcEvent312(IrcEvent *event);          // RPL_WHOISSERVER
    Q_INVOKABLE void processIrcEvent313(IrcEvent *event);          // RPL_WHOISOPERATOR
    Q_INVOKABLE void processIrcEvent315(IrcEvent *event);          // RPL_ENDOFWHO
    Q_INVOKABLE void processIrcEvent317(IrcEvent *event);          // RPL_WHOISIDLE
    Q_INVOKABLE void processIrcEvent322(IrcEvent *event);          // RPL_LIST
    Q_INVOKABLE void processIrcEvent323(IrcEvent *event);          // RPL_LISTEND
    Q_INVOKABLE void processIrcEvent324(IrcEvent *event);          // RPL_CHANNELMODEIS
    Q_INVOKABLE void processIrcEvent331(IrcEvent *event);          // RPL_NOTOPIC
    Q_INVOKABLE void processIrcEvent332(IrcEvent *event);          // RPL_TOPIC
    Q_INVOKABLE void processIrcEvent352(IrcEvent *event);          // RPL_WHOREPLY
    Q_INVOKABLE void processIrcEvent353(IrcEvent *event);          // RPL_NAMREPLY
    Q_INVOKABLE void processIrcEvent432(IrcEventNumeric *event);   // ERR_ERRONEUSNICKNAME
    Q_INVOKABLE void processIrcEvent433(IrcEventNumeric *event);   // ERR_NICKNAMEINUSE
    Q_INVOKABLE void processIrcEvent437(IrcEventNumeric *event);   // ERR_UNAVAILRESOURCE

    // Q_INVOKABLE void processIrcEvent(IrcEvent *event);

    /* CTCP handlers */
    Q_INVOKABLE void processCtcpEvent(CtcpEvent *event);

    Q_INVOKABLE void handleCtcpAction(CtcpEvent *event);
    Q_INVOKABLE void handleCtcpClientinfo(CtcpEvent *event);
    Q_INVOKABLE void handleCtcpPing(CtcpEvent *event);
    Q_INVOKABLE void handleCtcpTime(CtcpEvent *event);
    Q_INVOKABLE void handleCtcpVersion(CtcpEvent *event);
    Q_INVOKABLE void defaultHandler(const QString &ctcpCmd, CtcpEvent *event);

signals:
    void newEvent(Event *event);

protected:
    bool checkParamCount(IrcEvent *event, int minParams);
    inline CoreNetwork *coreNetwork(NetworkEvent *e) const { return qobject_cast<CoreNetwork *>(e->network()); }
    void tryNextNick(NetworkEvent *e, const QString &errnick, bool erroneous = false);

private slots:
    //! Joins after a netsplit
    /** This slot handles a bulk-join after a netsplit is over
      * \param net     The network
      * \param channel The channel the users joined
      * \param users   The list of users that joind the channel
      * \param modes   The list of modes the users get set
      * \param quitMessage The message we received when the netsplit occured
      */
    void handleNetsplitJoin(Network *net, const QString &channel, const QStringList &users, const QStringList &modes, const QString &quitMessage);

    //! Quits after a netsplit
    /** This slot handles a bulk-quit after a netsplit occured
      * \param net     The network
      * \param channel The channel the users quitted
      * \param users   The list of users that got split
      * \param quitMessage The message we received when the netsplit occured
      */
    void handleNetsplitQuit(Network *net, const QString &channel, const QStringList &users, const QString &quitMessage);

    //! Netsplit finished
    /** This slot deletes the netsplit object that sent the finished() signal
      */
    void handleNetsplitFinished();

    void handleEarlyNetsplitJoin(Network *net, const QString &channel, const QStringList &users, const QStringList &modes);

    //! Destroy any existing netsplits
    /** This slot deletes all netsplit objects
      * Used to get rid of existing netsplits on network reconnect
      * \param network The network we want to clear
      */
    void destroyNetsplits(NetworkId network);

private:
    CoreSession *_coreSession;

    // structure to organize netsplits
    // key: quit message
    // value: the corresponding netsplit object
    QHash<Network *, QHash<QString, Netsplit *> > _netsplits;
};


#endif
