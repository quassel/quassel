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

#ifndef _CHANNELWIDGETINPUT_H_
#define _CHANNELWIDGETINPUT_H_

#include <QtCore>
#include <QtGui>
#include "tabcompleter.h"

class ChannelWidgetInput : public QLineEdit {
  Q_OBJECT

  public:
    ChannelWidgetInput(QWidget *parent = 0);
    ~ChannelWidgetInput();
    
  protected:
    virtual bool event(QEvent *);
    virtual void keyPressEvent(QKeyEvent * event);

  private slots:
    void enter();

  public slots:
    void updateNickList(QStringList);
    
  signals:
    void nickListUpdated(QStringList);
    
  private:
    qint32 idx;
    QStringList history;
    QStringList nickList;

    TabCompleter *tabComplete;
};

#endif
