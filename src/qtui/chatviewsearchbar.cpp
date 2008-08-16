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

#include "chatviewsearchbar.h"

#include <QAction>

ChatViewSearchBar::ChatViewSearchBar(QWidget *parent)
  : QWidget(parent)
{
  ui.setupUi(this);
  layout()->setContentsMargins(0, 0, 0, 0);

  ui.searchUpButton->setEnabled(false);
  ui.searchDownButton->setEnabled(false);

  _toggleViewAction = new QAction(tr("Show search bar"), this);
  _toggleViewAction->setCheckable(true);
  _toggleViewAction->setChecked(false);
  connect(_toggleViewAction, SIGNAL(toggled(bool)),
	  this, SLOT(setVisible(bool)));
  setVisible(false);

  connect(ui.hideButton, SIGNAL(clicked()),
	  _toggleViewAction, SLOT(toggle()));
}

void ChatViewSearchBar::setVisible(bool visible) {
  QWidget::setVisible(visible);
  ui.searchEditLine->clear();
}
