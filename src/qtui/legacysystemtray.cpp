/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
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

#include <QIcon>

#include "legacysystemtray.h"
#include "mainwin.h"
#include "qtui.h"

LegacySystemTray::LegacySystemTray(QWidget *parent)
    : SystemTray(parent)
{
#ifndef HAVE_KDE4
    _trayIcon = new QSystemTrayIcon(associatedWidget());
#else
    _trayIcon = new KSystemTrayIcon(associatedWidget());
    // We don't want to trigger a minimize if a highlight is pending, so we brutally remove the internal connection for that
    disconnect(_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
        _trayIcon, SLOT(activateOrHide(QSystemTrayIcon::ActivationReason)));
#endif
#ifndef Q_OS_MAC
    connect(_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
        SLOT(onActivated(QSystemTrayIcon::ActivationReason)));
#endif
    connect(_trayIcon, SIGNAL(messageClicked()),
        SLOT(onMessageClicked()));

    _trayIcon->setContextMenu(trayMenu());
    _trayIcon->setVisible(false);

    setMode(Mode::Legacy);

    connect(this, SIGNAL(visibilityChanged(bool)), this, SLOT(onVisibilityChanged(bool)));
    connect(this, SIGNAL(modeChanged(Mode)), this, SLOT(onModeChanged(Mode)));
    connect(this, SIGNAL(toolTipChanged(QString, QString)), SLOT(updateToolTip()));
    connect(this, SIGNAL(iconsChanged()), this, SLOT(updateIcon()));
    connect(this, SIGNAL(currentIconNameChanged()), this, SLOT(updateIcon()));

    updateIcon();
    updateToolTip();
}


bool LegacySystemTray::isSystemTrayAvailable() const
{
    return mode() == Mode::Legacy
            ? QSystemTrayIcon::isSystemTrayAvailable()
            : SystemTray::isSystemTrayAvailable();
}


void LegacySystemTray::onVisibilityChanged(bool isVisible)
{
    if (mode() == Legacy) {
        _trayIcon->setVisible(isVisible);
    }
}


void LegacySystemTray::onModeChanged(Mode mode)
{
    if (mode == Mode::Legacy) {
        _trayIcon->setVisible(isVisible());
    }
    else {
        _trayIcon->hide();
    }
}


void LegacySystemTray::updateIcon()
{
    QString iconName = (state() == NeedsAttention) ? currentAttentionIconName() : currentIconName();
    _trayIcon->setIcon(QIcon::fromTheme(iconName, QIcon{QString{":/icons/hicolor/24x24/status/%1.svg"}.arg(iconName)}));
}


void LegacySystemTray::updateToolTip()
{
#if defined Q_OS_MAC || defined Q_OS_WIN
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


void LegacySystemTray::onActivated(QSystemTrayIcon::ActivationReason reason)
{
    activate((SystemTray::ActivationReason)reason);
}


void LegacySystemTray::onMessageClicked()
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
