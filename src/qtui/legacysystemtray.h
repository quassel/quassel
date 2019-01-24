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

#pragma once

#ifndef QT_NO_SYSTEMTRAYICON

#include <QString>

#ifdef HAVE_KDE4
#  include <KSystemTrayIcon>
#else
#  include <QSystemTrayIcon>
#endif

#include "systemtray.h"

class LegacySystemTray : public SystemTray
{
    Q_OBJECT

public:
    explicit LegacySystemTray(QWidget *parent);

    bool isSystemTrayAvailable() const override;

public slots:
    void showMessage(const QString &title, const QString &message, MessageIcon icon = Information, int msTimeout = 10000, uint notificationId = 0) override;
    void closeMessage(uint notificationId) override;

private slots:
    void onModeChanged(Mode mode);
    void onVisibilityChanged(bool isVisible);

    void onActivated(QSystemTrayIcon::ActivationReason);
    void onMessageClicked();

    void updateIcon();
    void updateToolTip();

private:
    uint _lastMessageId {0};

#ifdef HAVE_KDE4
    KSystemTrayIcon *_trayIcon;
#else
    QSystemTrayIcon *_trayIcon;
#endif
};

#endif /* QT_NO_SYSTEMTRAYICON */
