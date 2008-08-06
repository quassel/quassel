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

#ifndef _QTUI_H_
#define _QTUI_H_

#include "qtuistyle.h"
#include "quasselui.h"

class MainWin;
class MessageModel;
class QtUiMessageProcessor;

//! This class encapsulates Quassel's Qt-based GUI.
/** This is basically a wrapper around MainWin, which is necessary because we cannot derive MainWin
 *  from both QMainWindow and AbstractUi (because of multiple inheritance of QObject).
 */
class QtUi : public AbstractUi {
  Q_OBJECT

  public:
    QtUi();
    ~QtUi();
    //void init();
    MessageModel *createMessageModel(QObject *parent);
    AbstractMessageProcessor *createMessageProcessor(QObject *parent);

    static QtUiStyle *style();

  public slots:
    void init();

  protected slots:
    void connectedToCore();
    void disconnectedFromCore();

  private:
    MainWin *mainWin;
    static QtUiStyle *_style;
};

#endif
