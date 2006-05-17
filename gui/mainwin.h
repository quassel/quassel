/***************************************************************************
 *   Copyright (C) 2005 by The Quassel Team                                *
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

#ifndef _MAINWIN_H_
#define _MAINWIN_H_

#include <QMainWindow>

class QMenu;
class QWorkspace;

class ServerListDlg;

class MainWin : public QMainWindow {
  Q_OBJECT

  public:
    MainWin();

  protected:

  private slots:
    void showServerList();
    
  private:
    void setupMenus();
    
    QWorkspace *workspace;
    QToolBar *toolBar;
    QMenu *fileMenu, *editMenu, *ircMenu, *serverMenu, *windowMenu, *helpMenu, *settingsMenu;
    QAction *quitAct, *serverListAct;
    QAction *aboutAct, *aboutQtAct;
    QAction *identitiesAct, *configAct;

    ServerListDlg *serverListDlg;
};

#endif
