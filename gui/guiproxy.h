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

#ifndef _GUIPROXY_H_
#define _GUIPROXY_H_

#include "../main/proxy_common.h"

#include <QObject>
#include <QVariant>

/** This class is the GUI side of the proxy. The GUI connects its signals and slots to it,
 *  and the calls are marshalled and sent to (or received and unmarshalled from) the CoreProxy.
 *  The connection function is defined in main/main_gui.cpp or main/main_mono.cpp.
 */
class GUIProxy : public QObject {
  Q_OBJECT

  private:
    void send(GUISignal, QVariant arg1 = QVariant(), QVariant arg2 = QVariant(), QVariant arg3 = QVariant());
    void recv(CoreSignal, QVariant arg1 = QVariant(), QVariant arg2 = QVariant(), QVariant arg3 = QVariant());

  public:
    static GUIProxy * init();

  public slots:
    void gsUserInput(QString);


  signals:
    void psCoreMessage(QString);


};

extern GUIProxy *guiProxy;



#endif
