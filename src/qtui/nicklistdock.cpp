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

#include "nicklistdock.h"
#include "qtuisettings.h"

#include <QAction>
#include <QDebug>
#include <QEvent>
#include <QAbstractButton>

NickListDock::NickListDock(const QString &title, QWidget *parent)
  : QDockWidget(title, parent)
{
  QAction *toggleView = toggleViewAction();
  disconnect(toggleView, SIGNAL(triggered(bool)), this, 0);

  foreach(QAbstractButton *button, findChildren<QAbstractButton *>()) {
    if(disconnect(button, SIGNAL(clicked()), this, SLOT(close())))
      connect(button, SIGNAL(clicked()), toggleView, SLOT(trigger()));
  }

  installEventFilter(this);

  toggleView->setChecked(QtUiSettings().value("ShowNickList", QVariant(true)).toBool());
}

NickListDock::~NickListDock() {
  QtUiSettings().setValue("ShowNickList", toggleViewAction()->isChecked());
}

bool NickListDock::eventFilter(QObject *watched, QEvent *event) {
  Q_UNUSED(watched)
  if(event->type() != QEvent::Hide && event->type() != QEvent::Show)
    return false;

  emit visibilityChanged(event->type() == QEvent::Show);

  return true;
}
