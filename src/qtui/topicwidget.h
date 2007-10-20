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

#ifndef _TOPICWIDGET_H_
#define _TOPICWIDGET_H_

#include <QWidget>

#include "ui_topicwidget.h"

class TopicWidget : public QWidget {
  Q_OBJECT
  Q_PROPERTY(QString topic READ topic WRITE setTopic STORED false)

public:
  TopicWidget(QWidget *parent = 0);
  virtual ~TopicWidget();

  QString topic() const;
  void setTopic(const QString &newtopic);
  
private:
  Ui::TopicWidget ui;
};


#endif
