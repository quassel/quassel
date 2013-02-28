/***************************************************************************
 *   Copyright (C) 2005-2013 by the Quassel Project                        *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This file is free software; you can redistribute it and/or modify     *
 *   it under the terms of the GNU Library General Public License (LGPL)   *
 *   as published by the Free Software Foundation; either version 2 of the *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef QT_NO_SYSTEMTRAYICON

#include "legacysystemtray.h"
#include "mainwin.h"
#include "qtui.h"

LegacySystemTray::LegacySystemTray(QWidget *parent)
    : SystemTray(parent),
    _blinkState(false),
    _lastMessageId(0)
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
        SLOT(on_messageClicked()));

    _blinkTimer.setInterval(500);
    _blinkTimer.setSingleShot(false);
    connect(&_blinkTimer, SIGNAL(timeout()), SLOT(on_blinkTimeout()));

    connect(this, SIGNAL(toolTipChanged(QString, QString)), SLOT(syncLegacyIcon()));
}


void LegacySystemTray::init()
{
    if (mode() == Invalid) // derived class hasn't set a mode itself
        setMode(Legacy);

    SystemTray::init();

    _trayIcon->setContextMenu(trayMenu());
}


void LegacySystemTray::syncLegacyIcon()
{
    _trayIcon->setIcon(stateIcon());

#if defined Q_WS_MAC || defined Q_WS_WIN
    QString tooltip = QString("%1").arg(toolTipTitle());
    if (!toolTipSubTitle().isEmpty())
        tooltip += QString("\n%1").arg(toolTipSubTitle());
#else
    QString tooltip = QString("<b>%1</b>").arg(toolTipTitle());
    if (!toolTipSubTitle().isEmpty())
        tooltip += QString("<br>%1").arg(toolTipSubTitle());
#endif

    _trayIcon->setToolTip(tooltip);
}


void LegacySystemTray::setVisible(bool visible)
{
    SystemTray::setVisible(visible);
    if (mode() == Legacy) {
        if (shouldBeVisible())
            _trayIcon->show();
        else
            _trayIcon->hide();
    }
}


bool LegacySystemTray::isVisible() const
{
    if (mode() == Legacy) {
        return _trayIcon->isVisible();
    }
    return SystemTray::isVisible();
}


void LegacySystemTray::setMode(Mode mode_)
{
    if (mode_ == mode())
        return;

    SystemTray::setMode(mode_);

    if (mode() == Legacy) {
        syncLegacyIcon();
        if (shouldBeVisible())
            _trayIcon->show();
        else
            _trayIcon->hide();
        if (state() == NeedsAttention)
            _blinkTimer.start();
    }
    else {
        _trayIcon->hide();
        _blinkTimer.stop();
    }
}


void LegacySystemTray::setState(State state_)
{
    State oldstate = state();
    SystemTray::setState(state_);
    if (oldstate != state()) {
        if (state() == NeedsAttention && mode() == Legacy && animationEnabled())
            _blinkTimer.start();
        else {
            _blinkTimer.stop();
            _blinkState = false;
        }
    }
    if (mode() == Legacy)
        _trayIcon->setIcon(stateIcon());
}


Icon LegacySystemTray::stateIcon() const
{
    if (mode() == Legacy && state() == NeedsAttention && !_blinkState)
        return SystemTray::stateIcon(Active);
    return SystemTray::stateIcon();
}


void LegacySystemTray::on_blinkTimeout()
{
    _blinkState = !_blinkState;
    _trayIcon->setIcon(stateIcon());
}


void LegacySystemTray::on_activated(QSystemTrayIcon::ActivationReason reason)
{
    activate((SystemTray::ActivationReason)reason);
}


void LegacySystemTray::on_messageClicked()
{
    emit messageClicked(_lastMessageId);
}


void LegacySystemTray::showMessage(const QString &title, const QString &message, SystemTray::MessageIcon icon, int msTimeout, uint id)
{
    // fancy stuff later: show messages in order
    // for now, we just show the last message
    _lastMessageId = id;
    _trayIcon->showMessage(title, message, (QSystemTrayIcon::MessageIcon)icon, msTimeout);
}


void LegacySystemTray::closeMessage(uint notificationId)
{
    Q_UNUSED(notificationId)

    // there really seems to be no sane way to close the bubble... :(
#ifdef Q_WS_X11
    showMessage("", "", NoIcon, 1);
#endif
}


#endif /* QT_NO_SYSTEMTRAYICON */
