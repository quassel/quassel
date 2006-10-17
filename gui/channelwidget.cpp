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

#include "channelwidget.h"

#include <QtGui>
#include <iostream>

ChannelWidget::ChannelWidget(QWidget *parent) : QWidget(parent) {
  ui.setupUi(this);
  //ui.lineEdit->setText("foobar");

/*  //ui.splitter->
  ui.textBrowser->setHtml("[17:21] <em>--> Dante has joined #quassel (~hurz@p1af2242.dip.t-dialin.net)</em><br>"
                          "[17:21] <em>--> Sput has joined #quassel (~Sput42@vincent.mindpool.net)</em><br>"
                          "[17:23] &lt;<b>Dante</b>&gt; Das sieht ja soweit schonmal Klasse aus!<br>"
                          "[17:23] &lt;<b>Sput</b>&gt; Find ich auch... schade dass es noch nix tut :p<br>"
                          "[17:24] &lt;<b>Dante</b>&gt; Das wird sich ja gottseidank bald ändern.<br>"
                          "[17:24] &lt;<b>Sput</b>&gt; Wollen wir's hoffen :D"
                          );
 ui.listWidget->addItem("@Dante");
 ui.listWidget->addItem("@Sput");
  */
  //connect(&core, SIGNAL(outputLine( const QString& )), ui.textBrowser, SLOT(insertPlainText(const QString &)));
  //connect(ui.lineEdit, SIGNAL(
  connect(&core, SIGNAL(outputLine( const QString& )), ui.textBrowser, SLOT(insertPlainText(const QString &)));
  connect(ui.lineEdit, SIGNAL(returnPressed()), this, SLOT(enterPressed()));
  connect(this, SIGNAL(inputLine( const QString& )), &core, SLOT(inputLine( const QString& )));
  core.start();
  core.connectToIrc("irc.quakenet.org", 6668);
}

void ChannelWidget::enterPressed() {
  emit inputLine(ui.lineEdit->text());
  ui.lineEdit->clear();
}
