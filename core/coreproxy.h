/***************************************************************************
 *   Copyright (C) 2005/06 by The Quassel Team                             *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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

#ifndef _COREPROXY_H_
#define _COREPROXY_H_

#include "proxy_common.h"

#include <QObject>
#include <QVariant>

/** This class is the Core side of the proxy. The Core connects its signals and slots to it,
 *  and the calls are marshalled and sent to (or received and unmarshalled from) the GUIProxy.
 *  The connection functions are defined in main/main_core.cpp or main/main_mono.cpp.
 */
class CoreProxy : public QObject {
  Q_OBJECT

  private:
    void send(CoreSignal, QVariant arg1 = QVariant(), QVariant arg2 = QVariant(), QVariant arg3 = QVariant());
    void recv(GUISignal, QVariant arg1 = QVariant(), QVariant arg2 = QVariant(), QVariant arg3 = QVariant());

  public:
    CoreProxy();

  public slots:
    void csCoreMessage(QString);


  signals:
    void gsUserInput(QString);
    void gsRequestConnect(QString, quint16);

  friend class GUIProxy;
};

extern CoreProxy *coreProxy;



#endif
