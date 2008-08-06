/***************************************************************************
*   Copyright (C) 2005-08 by the Quassel Project                          *
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

#include "qtuimessageprocessor.h"

#include "client.h"
#include "clientsettings.h"
#include "identity.h"
#include "messagemodel.h"
#include "network.h"

QtUiMessageProcessor::QtUiMessageProcessor(QObject *parent) : AbstractMessageProcessor(parent) {
  _processing = false;
  _processMode = TimerBased;
  _processTimer.setInterval(0);
  connect(&_processTimer, SIGNAL(timeout()), this, SLOT(processNextMessage()));
}

void QtUiMessageProcessor::reset() {
  if(processMode() == TimerBased) {
    if(_processTimer.isActive()) _processTimer.stop();
    _processing = false;
    _currentBatch.clear();
    _processQueue.clear();
  }
}

void QtUiMessageProcessor::process(Message &msg) {
  checkForHighlight(msg);
  Client::messageModel()->insertMessage(msg);
  postProcess(msg);
}

void QtUiMessageProcessor::process(QList<Message> &msgs) {
  _processQueue.append(msgs);
  if(!isProcessing()) startProcessing();
}

void QtUiMessageProcessor::startProcessing() {
  if(processMode() == TimerBased) {
    if(_currentBatch.isEmpty() && _processQueue.isEmpty()) return;
    _processing = true;
    if(!_processTimer.isActive()) _processTimer.start();
  }
}

void QtUiMessageProcessor::processNextMessage() {
  if(_currentBatch.isEmpty()) {
    if(_processQueue.isEmpty()) {
      _processTimer.stop();
      _processing = false;
      return;
    }
    _currentBatch = _processQueue.takeFirst();
  }
  Message msg = _currentBatch.takeFirst();
  process(msg);
}

// TODO optimize checkForHighlight
void QtUiMessageProcessor::checkForHighlight(Message &msg) {
  if(!((msg.type() & (Message::Plain | Message::Notice | Message::Action)) && !(msg.flags() & Message::Self)))
    return;

  NotificationSettings notificationSettings;
  const Network *net = Client::network(msg.bufferInfo().networkId());
  if(net && !net->myNick().isEmpty()) {
    QStringList nickList;
    if(notificationSettings.highlightNick() == NotificationSettings::CurrentNick) {
      nickList << net->myNick();
    } else if(notificationSettings.highlightNick() == NotificationSettings::AllNicks) {
      const Identity *myIdentity = Client::identity(net->identity());
      if(myIdentity)
        nickList = myIdentity->nicks();
    }
    foreach(QString nickname, nickList) {
      QRegExp nickRegExp("^(.*\\W)?" + QRegExp::escape(nickname) + "(\\W.*)?$");
      if(nickRegExp.exactMatch(msg.contents())) {
        msg.setFlags(msg.flags() | Message::Highlight);
        return;
      }
    }

    foreach(QVariant highlight, notificationSettings.highlightList()) {
      QVariantMap highlightRule = highlight.toMap();
      if(!highlightRule["enable"].toBool())
        continue;
      Qt::CaseSensitivity caseSensitivity = highlightRule["cs"].toBool() ? Qt::CaseSensitive : Qt::CaseInsensitive;
      QString name = highlightRule["name"].toString();
      QRegExp userRegExp;
      if(highlightRule["regex"].toBool()) {
        userRegExp = QRegExp(name, caseSensitivity);
      } else {
        userRegExp = QRegExp("^(.*\\W)?" + QRegExp::escape(name) + "(\\W.*)?$", caseSensitivity);
      }
      if(userRegExp.exactMatch(msg.contents())) {
        msg.setFlags(msg.flags() | Message::Highlight);
        return;
      }
    }
  }
}
