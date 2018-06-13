/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This contains code from KStatusNotifierItem, part of the KDE libs     *
 *   Copyright (C) 2009 Marco Martin <notmart@gmail.com>                   *
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

#ifdef HAVE_DBUS

#include <QDBusError>
#include <QHash>
#include <QString>

#if QT_VERSION >= 0x050000
#  include <QTemporaryDir>
#endif

#include "notificationsclient.h"
#include "systemtray.h"
#include "statusnotifierwatcher.h"

#ifdef QT_NO_SYSTEMTRAYICON
#  define StatusNotifierItemParent SystemTray
#else
#  define StatusNotifierItemParent LegacySystemTray
#  include "legacysystemtray.h"
#endif

class StatusNotifierItemDBus;

class StatusNotifierItem : public StatusNotifierItemParent
{
    Q_OBJECT

public:
    explicit StatusNotifierItem(QWidget *parent);

    bool isSystemTrayAvailable() const override;

public slots:
    void showMessage(const QString &title, const QString &message, MessageIcon icon = Information, int msTimeout = 10000, uint notificationId = 0) override;
    void closeMessage(uint notificationId) override;

protected:
    QString title() const;
    QString iconName() const;
    QString attentionIconName() const;
    QString toolTipIconName() const;
    QString iconThemePath() const;
    QString menuObjectPath() const;

    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void activated(const QPoint &pos);
    void serviceChange(const QString &name, const QString &oldOwner, const QString &newOwner);
    void checkForRegisteredHosts();
    void onDBusError(const QDBusError &error);

    void notificationClosed(uint id, uint reason);
    void notificationInvoked(uint id, const QString &action);

    void refreshIcons();

    void onModeChanged(Mode mode);
    void onStateChanged(State state);
    void onVisibilityChanged(bool isVisible);

private:
    void registerToWatcher();

    StatusNotifierItemDBus *_statusNotifierItemDBus{nullptr};
    org::kde::StatusNotifierWatcher *_statusNotifierWatcher{nullptr};
    org::freedesktop::Notifications *_notificationsClient{nullptr};
    bool _notificationsClientSupportsMarkup{false};
    bool _notificationsClientSupportsActions{false};
    quint32 _lastNotificationsDBusId{0};
    QHash<uint, uint> _notificationsIdMap; ///< Maps our own notification ID to the D-Bus one

    QString _iconThemePath;
    QString _menuObjectPath;

#if QT_VERSION >= 0x050000
    QTemporaryDir _iconThemeDir;
#endif

    friend class StatusNotifierItemDBus;
};

#endif /* HAVE_DBUS */
