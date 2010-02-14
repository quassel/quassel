/***************************************************************************
 *   Copyright (C) 2005-2010 by the Quassel Project                        *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This contains code from KStatusNotifierItem, part of the KDE libs     *
 *   Copyright (C) 2009 Marco Martin <notmart@gmail.com>                   *
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

#ifdef Q_WS_WIN
  _dwTickCount = 0;
  associatedWidget()->installEventFilter(this);
#endif

  qApp->installEventFilter(this);
}

SystemTray::~SystemTray() {
#ifdef Q_WS_WIN
  associatedWidget()->removeEventFilter(this);
#endif

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
    toggleMainWidget();
  }
}

bool SystemTray::eventFilter(QObject *obj, QEvent *event) {
  if(event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonRelease) {
    _inhibitActivation = false;
  }
#ifdef Q_WS_WIN
  if(obj == associatedWidget() && event->type() == QEvent::ActivationChange) {
    _dwTickCount = GetTickCount();
  }
#endif
  return QObject::eventFilter(obj, event);
}

// Code taken from KStatusNotifierItem for handling minimize/restore

bool SystemTray::checkVisibility(bool perform) {
#ifdef Q_WS_WIN
  // the problem is that we lose focus when the systray icon is activated
  // and we don't know the former active window
  // therefore we watch for activation event and use our stopwatch :)
  if(GetTickCount() - _dwTickCount < 300) {
    // we were active in the last 300ms -> hide it
    minimizeRestore(false);
  } else {
    minimizeRestore(true);
  }

#elif defined(HAVE_KDE) && defined(Q_WS_X11)
  KWindowInfo info1 = KWindowSystem::windowInfo(associatedWidget()->winId(), NET::XAWMState | NET::WMState | NET::WMDesktop);
  // mapped = visible (but possibly obscured)
  bool mapped = (info1.mappingState() == NET::Visible) && !info1.isMinimized();

  //    - not mapped -> show, raise, focus
  //    - mapped
  //        - obscured -> raise, focus
  //        - not obscured -> hide
  //info1.mappingState() != NET::Visible -> window on another desktop?
  if(!mapped) {
    if(perform)
      minimizeRestore(true);
    return true;

  } else {
    QListIterator< WId > it (KWindowSystem::stackingOrder());
    it.toBack();
    while(it.hasPrevious()) {
      WId id = it.previous();
      if(id == associatedWidget()->winId())
        break;

      KWindowInfo info2 = KWindowSystem::windowInfo(id, NET::WMDesktop | NET::WMGeometry | NET::XAWMState | NET::WMState | NET::WMWindowType);

      if(info2.mappingState() != NET::Visible)
        continue; // not visible on current desktop -> ignore

      if(!info2.geometry().intersects(associatedWidget()->geometry()))
        continue; // not obscuring the window -> ignore

      if(!info1.hasState(NET::KeepAbove) && info2.hasState(NET::KeepAbove))
        continue; // obscured by window kept above -> ignore

      NET::WindowType type = info2.windowType(NET::NormalMask | NET::DesktopMask
                                              | NET::DockMask | NET::ToolbarMask | NET::MenuMask | NET::DialogMask
                                              | NET::OverrideMask | NET::TopMenuMask | NET::UtilityMask | NET::SplashMask);

      if(type == NET::Dock || type == NET::TopMenu)
        continue; // obscured by dock or topmenu -> ignore

      if(perform) {
        KWindowSystem::raiseWindow(associatedWidget()->winId());
        KWindowSystem::activateWindow(associatedWidget()->winId());
      }
      return true;
    }

    //not on current desktop?
    if(!info1.isOnCurrentDesktop()) {
      if(perform)
        KWindowSystem::activateWindow(associatedWidget()->winId());
      return true;
    }

    if(perform)
      minimizeRestore(false); // hide
    return false;
  }
#else

  if(!associatedWidget()->isVisible() || associatedWidget()->isMinimized()) {
    if(perform)
      minimizeRestore(true);
    return true;
  } else {
    if(perform)
      minimizeRestore(false);
    return false;
  }

#endif

  return true;
}

void SystemTray::minimizeRestore(bool show) {
  if(show)
    GraphicalUi::activateMainWidget();
  else {
    if(isSystemTrayAvailable()) {
      if(!isVisible())
        setVisible();
      GraphicalUi::hideMainWidget();
    }
  }
}

void SystemTray::hideMainWidget() {
  minimizeRestore(false);
}

void SystemTray::toggleMainWidget() {
  checkVisibility(true);
}
