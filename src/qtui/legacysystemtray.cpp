/***************************************************************************
 *   Copyright (C) 2005-2019 by the Quassel Project                        *
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

#    include "legacysystemtray.h"

#    include "icon.h"
#    include "mainwin.h"
#    include "qtui.h"

LegacySystemTray::LegacySystemTray(QWidget* parent)
    : SystemTray(parent)
    , _trayIcon{new QSystemTrayIcon(associatedWidget())}
{
#    ifndef Q_OS_MAC
    connect(_trayIcon, &QSystemTrayIcon::activated, this, &LegacySystemTray::onActivated);
#    endif
    connect(_trayIcon, &QSystemTrayIcon::messageClicked, this, &LegacySystemTray::onMessageClicked);

    _trayIcon->setContextMenu(trayMenu());
    _trayIcon->setVisible(false);

    setMode(Mode::Legacy);

    connect(this, &SystemTray::visibilityChanged, this, &LegacySystemTray::onVisibilityChanged);
    connect(this, &SystemTray::modeChanged, this, &LegacySystemTray::onModeChanged);
    connect(this, &SystemTray::toolTipChanged, this, &LegacySystemTray::updateToolTip);
    connect(this, &SystemTray::iconsChanged, this, &LegacySystemTray::updateIcon);
    connect(this, &SystemTray::currentIconNameChanged, this, &LegacySystemTray::updateIcon);

    updateIcon();
    updateToolTip();
}

bool LegacySystemTray::isSystemTrayAvailable() const
{
    return mode() == Mode::Legacy ? QSystemTrayIcon::isSystemTrayAvailable() : SystemTray::isSystemTrayAvailable();
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
    _trayIcon->setIcon(icon::get(iconName, QString{":/icons/hicolor/24x24/status/%1.svg"}.arg(iconName)));
}

void LegacySystemTray::updateToolTip()
{
#    if defined Q_OS_MAC || defined Q_OS_WIN
    QString tooltip = QString("%1").arg(toolTipTitle());
    if (!toolTipSubTitle().isEmpty())
        tooltip += QString("\n%1").arg(toolTipSubTitle());
#    else
    QString tooltip = QString("<b>%1</b>").arg(toolTipTitle());
    if (!toolTipSubTitle().isEmpty())
        tooltip += QString("<br>%1").arg(toolTipSubTitle());
#    endif

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

void LegacySystemTray::showMessage(const QString& title, const QString& message, SystemTray::MessageIcon icon, int msTimeout, uint id)
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
#    ifdef Q_WS_X11
    showMessage("", "", NoIcon, 1);
#    endif
}

#endif /* QT_NO_SYSTEMTRAYICON */
