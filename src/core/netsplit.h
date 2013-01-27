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

#ifndef NETSPLIT_H
#define NETSPLIT_H

#include <QTimer>
#include <QHash>
#include <QPair>
#include <QStringList>

class Network;

class Netsplit : public QObject
{
    Q_OBJECT
public:
    Netsplit(Network *network, QObject *parent = 0);

    inline Network *network() const { return _network; }

    //! Add a user to the netsplit
    /** Call this method if you noticed a netsplit.
      * \note This method doesn't check if it really is a netsplit.
      * \note Check with isNetsplit(const QString &quitMessage) before calling it!
      *
      * \param sender   The sender string of the quitting user
      * \param channels The channels that user shared with us
      * \param msg      The quit message
      */
    void userQuit(const QString &sender, const QStringList &channels, const QString &msg);

    //! Remove a user from the netsplit
    /** Call this method if a user joined after a netsplit occured.
      *
      * \param sender   The sender string of the joined user
      * \param channel The channel that user shares with us
      * \return true if user was found in the netsplit
      */
    bool userJoined(const QString &sender, const QString &channel);

    //! Check if user has joined since netsplit
    /** This method shows if a user has already joined after being hit by netsplit
      * \note The method doesn't check if the user was recorded in the netsplit!
      *
      * \param sender   The sender string of the user
      * \param channel  The channel the user shares with us
      * \return true if user joined after a netsplit
      */
    bool userAlreadyJoined(const QString &sender, const QString &channel);

    //! Add mode to user
    /** Use this method to buffer userspecific channel modes until netsplitJoin is emitted.
      *
      * \param sender   The sender string of the user
      * \param channel  The channel the user shares with us
      * \return true if user joined after a netsplit
      */
    void addMode(const QString &sender, const QString &channel, const QString &mode);

    //! Check if a string matches the criteria for a netsplit
    /** \param quitMessage The message to be checked
      * \return true if the message is a netsplit
      */
    static bool isNetsplit(const QString &quitMessage);

signals:
    //! A bulk-join of netsplitted users timed out
    /** Whenever the internal join-timer times out, we consider the bulk-join to be finished and emit that signal
      * for every channel. This is the end of a netsplit.
      * \param net     The network
      * \param channel The IRC channel
      * \param users   A list of all users that joined that channel
      * \param modes   A list of all modes the users got set after joining again
      * \param quitMessage The Quitmessage and thus the servers that got split
      */
    void netsplitJoin(Network *net, const QString &channel, const QStringList &users, const QStringList &modes, const QString &quitMessage);

    //! A (probably bulk-) join of netsplitted users.
    /** If users hit by the split joined before the netsplit is considered over, join the users with a normal join.
      * \param net     The network
      * \param channel The IRC channel
      * \param users   A list of all users that joined that channel
      * \param modes   A list of all modes the users got set after joining again
      */
    void earlyJoin(Network *net, const QString &channel, const QStringList &users, const QStringList &modes);

    //! A bulk-quit of netsplitted users timed out
    /** Whenever the internal quit-timer times out, we consider the bulk-quit to be finished and emit that signal
      * for every channel.
      * \param net     The network
      * \param channel The IRC channel
      * \param users   A list of all users that quitted in that channel
      * \param quitMessage The Quitmessage and thus the servers that got split
      */
    void netsplitQuit(Network *net, const QString &channel, const QStringList &users, const QString &quitMessage);

    //! The Netsplit is considered finished
    /** This signal is emitted right after all netsplitJoin signals have been sent or whenever the
      * internal timer signals a timeout.
      * Simply delete the object and remove it from structures when you receive that signal.
      */
    void finished();

private slots:
    void joinTimeout();
    void quitTimeout();

private:
    Network *_network;
    QString _quitMsg;
    // key: channel name
    // value: senderstring, list of modes
    QHash<QString, QPair<QStringList, QStringList> > _joins;
    QHash<QString, QStringList> _quits;
    QHash<QString, QStringList> _quitsWithMessageSent;
    bool _sentQuit;
    QTimer _joinTimer;
    QTimer _quitTimer;
    QTimer _discardTimer;
    int _joinCounter;
    int _quitCounter;
};


#endif // NETSPLIT_H
