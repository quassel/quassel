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

#ifndef _CORECONNECTDLG_H
#define _CORECONNECTDLG_H

#include "ui_coreconnectdlg.h"

class CoreConnectDlg: public QDialog {
  Q_OBJECT

  public:
    CoreConnectDlg(QWidget *);
    QVariant getCoreState();

  private slots:
    void hostEditChanged(QString);
    void hostSelected();

    void coreConnected();
    void coreConnectionError(QString);
    void updateProgressBar(quint32 bytes, quint32 avail);
    void recvCoreState(QVariant);

  private:
    Ui::CoreConnectDlg ui;
    QVariant coreState;

    void setStartState();
};

#endif
