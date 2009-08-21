/***************************************************************************
 *   Copyright (C) 2005/06 by the Quassel Project                          *
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

#ifndef TOPICWIDGET_H_
#define TOPICWIDGET_H_

#include "abstractitemview.h"

#include "ui_topicwidget.h"

class TopicWidget : public AbstractItemView {
  Q_OBJECT

public:
  TopicWidget(QWidget *parent = 0);

  void setTopic(const QString &newtopic);

  virtual bool eventFilter(QObject *obj, QEvent *event);

protected slots:
  virtual void currentChanged(const QModelIndex &current, const QModelIndex &previous);
  virtual void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);

private slots:
  void on_topicLineEdit_textEntered();
  void on_topicEditButton_clicked();
  void switchEditable();
  void switchPlain();

private:
  Ui::TopicWidget ui;

  QString _topic;
};


#endif
