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

#ifndef QT_NO_SYSTEMTRAYICON

#include "legacysystemtray.h"
#include "qtui.h"

LegacySystemTray::LegacySystemTray(QWidget *parent)
  : SystemTray(parent),
  _blinkState(false),
  _isVisible(true)
{
#ifndef HAVE_KDE
  _trayIcon = new QSystemTrayIcon(associatedWidget());
#else
  _trayIcon = new KSystemTrayIcon(associatedWidget());
  // We don't want to trigger a minimize if a highlight is pending, so we brutally remove the internal connection for that
  disconnect(_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
                 _trayIcon, SLOT(activateOrHide(QSystemTrayIcon::ActivationReason)));
#endif
#ifndef Q_WS_MAC
  connect(_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
                     SLOT(on_activated(QSystemTrayIcon::ActivationReason)));
#endif
  connect(_trayIcon, SIGNAL(messageClicked()),
                     SIGNAL(messageClicked()));

  _blinkTimer.setInterval(500);
  _blinkTimer.setSingleShot(false);
  connect(&_blinkTimer, SIGNAL(timeout()), SLOT(on_blinkTimeout()));
}

void LegacySystemTray::init() {
  if(mode() == Invalid) // derived class hasn't set a mode itself
    setMode(Legacy);

  SystemTray::init();

  _trayIcon->setContextMenu(trayMenu());
}

void LegacySystemTray::syncLegacyIcon() {
  _trayIcon->setIcon(stateIcon());
  _trayIcon->setToolTip(toolTipTitle());
}

void LegacySystemTray::setVisible(bool visible) {
  _isVisible = visible;
  if(mode() == Legacy) {
    if(visible)
      _trayIcon->show();
    else
      _trayIcon->hide();
  }
}

bool LegacySystemTray::isVisible() const {
  if(mode() == Legacy) {
    return _trayIcon->isVisible();
  }
  return false;
}

void LegacySystemTray::setMode(Mode mode_) {
  SystemTray::setMode(mode_);

  if(mode() == Legacy) {
    syncLegacyIcon();
    if(_isVisible)
      _trayIcon->show();
    if(state() == NeedsAttention)
      _blinkTimer.start();
  } else {
    _trayIcon->hide();
    _blinkTimer.stop();
  }
}

void LegacySystemTray::setState(State state_) {
  State oldstate = state();
  SystemTray::setState(state_);
  if(oldstate != state()) {
    if(state() == NeedsAttention && mode() == Legacy)
      _blinkTimer.start();
    else {
      _blinkTimer.stop();
      _blinkState = false;
    }
  }
  if(mode() == Legacy)
    _trayIcon->setIcon(stateIcon());
}

Icon LegacySystemTray::stateIcon() const {
  if(mode() == Legacy && state() == NeedsAttention && !_blinkState)
    return SystemTray::stateIcon(Active);
  return SystemTray::stateIcon();
}

void LegacySystemTray::on_blinkTimeout() {
  _blinkState = !_blinkState;
  _trayIcon->setIcon(stateIcon());
}

void LegacySystemTray::on_activated(QSystemTrayIcon::ActivationReason reason) {
  activate((SystemTray::ActivationReason)reason);
}

void LegacySystemTray::showMessage(const QString &title, const QString &message, SystemTray::MessageIcon icon, int millisecondsTimeoutHint) {
  _trayIcon->showMessage(title, message, (QSystemTrayIcon::MessageIcon)icon, millisecondsTimeoutHint);
}

#endif /* QT_NO_SYSTEMTRAYICON */
