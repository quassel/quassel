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

#include "mainwidget.h"

#include "buffer.h"

MainWidget::MainWidget(QWidget *parent) : QWidget(parent) {
  ui.setupUi(this);

//  ui.bufferLeft->setIcon(QIcon(":icon/left"));
//  ui.bufferRight->setIcon(QIcon(":icon/right"));
  //ui.bufferLeft->setIconSize(QSize(10, 10));
  //ui.bufferRight->setIconSize(QSize(10, 10));
  //ui.bufferLeft->setMaximumSize(QSize(10,10));
  //ui.bufferRight->setMaximumSize(QSize(10,10));
}

MainWidget::~MainWidget() {



}

void MainWidget::setBuffer(Buffer *b) {
  //  TODO update topic if changed; handle status buffer display
  QString title = QString("%1 (%2): \"%3\"").arg(b->displayName()).arg(b->networkName()).arg(b->topic());
  ui.topicBar->setContents(title);

}
