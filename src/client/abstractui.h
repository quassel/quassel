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

#ifndef QUASSELUI_H
#define QUASSELUI_H

#include <QObject>
#include "message.h"

class MessageFilter;
class MessageModel;
class AbstractMessageProcessor;
class AbstractActionProvider;

class QAction;
class QMenu;

class AbstractUi : public QObject {
  Q_OBJECT

  public:
    AbstractUi(QObject *parent = 0);
    virtual ~AbstractUi() {};
    virtual void init() = 0;  // called after the client is initialized
    virtual MessageModel *createMessageModel(QObject *parent) = 0;
    virtual AbstractMessageProcessor *createMessageProcessor(QObject *parent) = 0;

    inline static bool isVisible() { return _visible; }
    inline static void setVisible(bool visible) { _visible = visible; }

  protected slots:
    virtual void connectedToCore() {}
    virtual void disconnectedFromCore() {}

  signals:
    void connectToCore(const QVariantMap &connInfo);
    void disconnectFromCore();

  private:
    static bool _visible;
};

#endif
