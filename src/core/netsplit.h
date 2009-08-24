/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef NETSPLIT_H
#define NETSPLIT_H

#include <QObject>
#include <QTimer>
#include <QHash>
#include <QStringList>

class Netsplit : public QObject
{
  Q_OBJECT
public:
  Netsplit();

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
    * \param channels The channels that user shares with us
    * \return true if user was found in the netsplit
    */
  bool userJoined(const QString &sender, const QString &channel);

  //! Check if a string matches the criteria for a netsplit
  /** \param quitMessage The message to be checked
    * \return true if the message is a netsplit
    */
  static bool isNetsplit(const QString &quitMessage);

signals:
  //! A bulk-join of netsplitted users timed out
  /** Whenever _joinTimer() times out, we consider the bulk-join to be finished and emit that signal
    * for every channel
    * \param channel The IRC channel
    * \param users   A list of all users that joined that channel
    * \param quitMessage The Quitmessage and thus the servers that got split
    */
  void netsplitJoin(const QString &channel, const QStringList &users, const QString &quitMessage);

  //! A bulk-quit of netsplitted users timed out
  /** Whenever _quitTimer() times out, we consider the bulk-quit to be finished and emit that signal
    * for every channel
    * \param channel The IRC channel
    * \param users   A list of all users that quitted in that channel
    * \param quitMessage The Quitmessage and thus the servers that got split
    */
  void netsplitQuit(const QString &channel, const QStringList &users, const QString &quitMessage);

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
  QString _quitMsg;
  QHash<QString, QStringList> _joins;
  QHash<QString, QStringList> _quits;
  bool _sentQuit;
  QTimer _joinTimer;
  QTimer _quitTimer;
  QTimer _discardTimer;
};

#endif // NETSPLIT_H
