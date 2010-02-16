/***************************************************************************
 *   Copyright (C) 2005-2010 by the Quassel Project                        *
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
#include <QMenu>

#include "systemtray.h"

#include "actioncollection.h"
#include "client.h"
#include "iconloader.h"
#include "qtui.h"

#ifdef HAVE_KDE
#  include <KWindowInfo>
#  include <KWindowSystem>
#endif

SystemTray::SystemTray(QWidget *parent)
: QObject(parent),
  _mode(Invalid),
  _state(Passive),
  _inhibitActivation(false),
  _passiveIcon(DesktopIcon("quassel_inactive")),
  _activeIcon(DesktopIcon("quassel")),
  _needsAttentionIcon(DesktopIcon("quassel_message")),
  _trayMenu(0),
  _associatedWidget(parent)
{
  Q_ASSERT(parent);

  qApp->installEventFilter(this);
}

SystemTray::~SystemTray() {
  _trayMenu->deleteLater();
}

QWidget *SystemTray::associatedWidget() const {
  return _associatedWidget;
}

void SystemTray::setTrayMenu(QMenu *menu) {
  if(menu)
    _trayMenu = menu;
  else
    _trayMenu = new QMenu();

  ActionCollection *coll = QtUi::actionCollection("General");

  _trayMenu->addAction(coll->action("ConnectCore"));
  _trayMenu->addAction(coll->action("DisconnectCore"));
  _trayMenu->addAction(coll->action("CoreInfo"));
#ifndef HAVE_KDE
  _trayMenu->addSeparator();
  _trayMenu->addAction(coll->action("Quit"));
#endif /* HAVE_KDE */
}

void SystemTray::setMode(Mode mode_) {
  if(mode_ != _mode) {
    _mode = mode_;
    if(_mode == Legacy) {
      _trayMenu->setWindowFlags(Qt::Popup);
    } else {
      _trayMenu->setWindowFlags(Qt::Window);
    }
  }
}

Icon SystemTray::stateIcon() const {
  return stateIcon(state());
}

Icon SystemTray::stateIcon(State state) const {
  switch(state) {
  case Passive:
    return _passiveIcon;
  case Active:
    return _activeIcon;
  case NeedsAttention:
    return _needsAttentionIcon;
  }
  return Icon();
}

void SystemTray::setState(State state) {
  if(_state != state) {
    _state = state;
  }
}

void SystemTray::setAlert(bool alerted) {
  if(alerted)
    setState(NeedsAttention);
  else
    setState(Client::isConnected() ? Active : Passive);
}

void SystemTray::setVisible(bool visible) {
  Q_UNUSED(visible)
}

void SystemTray::setToolTip(const QString &title, const QString &subtitle) {
  _toolTipTitle = title;
  _toolTipSubTitle = subtitle;
  emit toolTipChanged(title, subtitle);
}

void SystemTray::showMessage(const QString &title, const QString &message, MessageIcon icon, int millisecondsTimeoutHint) {
  Q_UNUSED(title)
  Q_UNUSED(message)
  Q_UNUSED(icon)
  Q_UNUSED(millisecondsTimeoutHint)
}

void SystemTray::activate(SystemTray::ActivationReason reason) {

  emit activated(reason);

  if(reason == Trigger && !isActivationInhibited()) {
    GraphicalUi::toggleMainWidget();
  }
}

bool SystemTray::eventFilter(QObject *obj, QEvent *event) {
  if(event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonRelease) {
    _inhibitActivation = false;
  }
  return QObject::eventFilter(obj, event);
}
