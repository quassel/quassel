/***************************************************************************
 *   Copyright (C) 2005-07 by The Quassel IRC Development Team             *
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

#ifndef _QTGUI_H_
#define _QTGUI_H_

#include "quasselui.h"
class MainWin;

//! This class encapsulates Quassel's Qt-based GUI.
/** This is basically a wrapper around MainWin, which is necessary because we cannot derive MainWin
 *  from both QMainWindow and AbstractUi (because of multiple inheritance of QObject).
 */
class QtGui : public AbstractUi {
  Q_OBJECT

  public:
    QtGui();
    ~QtGui();
    void init();
    AbstractUiMsg *layoutMsg(const Message &);

  protected slots:
    void connectedToCore();
    void disconnectedFromCore();

  private:
    MainWin *mainWin;
};

#endif
