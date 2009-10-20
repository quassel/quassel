/***************************************************************************
*   Copyright (C) 2005-09 by the Quassel Project                          *
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
#include "iconloader.h"
#include "qtui.h"
#include "qtuisettings.h"

SystemTray::SystemTray(QObject *parent)
: QObject(parent),
  _state(Inactive),
  _alert(false),
  _inhibitActivation(false),
  _currentIdx(0)
{
  loadAnimations();
  _currentIdx = _idxOffEnd;

#ifndef HAVE_KDE
  _trayIcon = new QSystemTrayIcon(_phases.at(_currentIdx), QtUi::mainWindow());
#else
  _trayIcon = new KSystemTrayIcon(_phases.at(_currentIdx), QtUi::mainWindow());
  // We don't want to trigger a minimize if a highlight is pending, so we brutally remove the internal connection for that
  disconnect(_trayIcon, SIGNAL(activated( QSystemTrayIcon::ActivationReason)),
             _trayIcon, SLOT(activateOrHide(QSystemTrayIcon::ActivationReason)));
#endif

  _animationTimer.setInterval(150);
  _animationTimer.setSingleShot(false);
  connect(&_animationTimer, SIGNAL(timeout()), SLOT(nextPhase()));

  ActionCollection *coll = QtUi::actionCollection("General");
  _trayMenu = _trayIcon->contextMenu();
  if (!_trayMenu)
    _trayMenu = new QMenu();
  _trayMenu->addAction(coll->action("ConnectCore"));
  _trayMenu->addAction(coll->action("DisconnectCore"));
  _trayMenu->addAction(coll->action("CoreInfo"));
#ifndef HAVE_KDE
  _trayMenu->addSeparator();
  _trayMenu->addAction(coll->action("Quit"));
#endif /* HAVE_KDE */

  _trayIcon->setContextMenu(_trayMenu);

  QtUiSettings s;
  if(s.value("UseSystemTrayIcon", QVariant(true)).toBool()) {
    _trayIcon->show();
  }

  qApp->installEventFilter(this);

#ifndef Q_WS_MAC
  connect(_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), SLOT(on_activated(QSystemTrayIcon::ActivationReason)));
#endif
  connect(_trayIcon, SIGNAL(messageClicked()), SIGNAL(messageClicked()));
}

SystemTray::~SystemTray() {
  _trayMenu->deleteLater();
}

void SystemTray::loadAnimations() {
// system tray icon size
#ifdef Q_WS_WIN
  const int size = 16;
#elif defined Q_WS_MAC
  const int size = 128;
#else
  const int size = 22;
#endif

  _phases.clear();

#ifdef HAVE_KDE
  KIconLoader *loader = KIconLoader::global();
#else
  IconLoader *loader = IconLoader::global();
#endif

  _idxOffStart = 0;
  QString fadeOffName("quassel_tray-fade-off-%1");
  for(int i = 2; i <= 10; i++)
    _phases.append(loader->loadIcon(fadeOffName.arg(i), IconLoader::Panel, size));
  _idxOffEnd = _idxOnStart = _phases.count() - 1;

  QString fadeOnName("quassel_tray-fade-on-%1");
  for(int i = 2; i <= 15; i++)
    _phases.append(loader->loadIcon(fadeOnName.arg(i), IconLoader::Panel, size));
  _idxOnEnd = _idxAlertStart = _phases.count() - 1;

  QString alertName("quassel_tray-alert-%1");
  for(int i = 1; i <= 10; i++)
    _phases.append(loader->loadIcon(alertName.arg(i), IconLoader::Panel, size));
}

void SystemTray::nextPhase() {
  if(_currentIdx == _idxOnEnd && !_alert && _state == Inactive)
    _currentIdx = _idxOffStart; // skip alert phases

  else if(++_currentIdx >= _phases.count()) {
    if(_alert)
      _currentIdx = _idxAlertStart;
    else
      if(_state == Active)
        _currentIdx = _idxOnEnd;
      else
        _currentIdx = _idxOffStart;
  }

  _trayIcon->setIcon(_phases.at(_currentIdx));

  if(_alert)
    return;

  if((_state == Active && _currentIdx == _idxOnEnd) || (_state == Inactive && _currentIdx == _idxOffEnd))
    _animationTimer.stop();
}

void SystemTray::setState(State state) {
  if(_state != state) {
    _state = state;
    if(state == Inactive && _alert)
      _alert = false;
    if(!_animationTimer.isActive())
      _animationTimer.start();
  }
}

void SystemTray::setAlert(bool alert) {
  if(_alert != alert) {
    _alert = alert;
    if(!_animationTimer.isActive())
      _animationTimer.start();
  }
}

void SystemTray::setIconVisible(bool visible) {
  if(visible)
    _trayIcon->show();
  else
    _trayIcon->hide();
}

void SystemTray::setToolTip(const QString &tip) {
  _trayIcon->setToolTip(tip);
}

void SystemTray::showMessage(const QString &title, const QString &message, QSystemTrayIcon::MessageIcon icon, int millisecondsTimeoutHint) {
  _trayIcon->showMessage(title, message, icon, millisecondsTimeoutHint);
}

bool SystemTray::eventFilter(QObject *obj, QEvent *event) {
  Q_UNUSED(obj);
  if(event->type() == QEvent::MouseButtonRelease) {
    _inhibitActivation = false;
  }
  return false;
}

void SystemTray::on_activated(QSystemTrayIcon::ActivationReason reason) {
  emit activated(reason);

  if(reason == QSystemTrayIcon::Trigger && !_inhibitActivation) {

#  ifdef HAVE_KDE
     // the slot is private, but meh, who cares :)
     QMetaObject::invokeMethod(_trayIcon, "activateOrHide", Q_ARG(QSystemTrayIcon::ActivationReason, QSystemTrayIcon::Trigger));
#  else
     QtUi::mainWindow()->toggleMinimizedToTray();
#  endif

  }
}
