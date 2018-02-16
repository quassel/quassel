/***************************************************************************
 *   Copyright (C) 2005-2016 by the Quassel Project                        *
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

#ifndef STATUSNOTIFIERITEM_H_
#define STATUSNOTIFIERITEM_H_

#ifdef HAVE_DBUS

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
    virtual ~StatusNotifierItem();

    virtual bool isSystemTrayAvailable() const;
    virtual bool isVisible() const;

public slots:
    virtual void setState(State state);
    virtual void setVisible(bool visible);
    virtual void showMessage(const QString &title, const QString &message, MessageIcon icon = Information, int msTimeout = 10000, uint notificationId = 0);
    virtual void closeMessage(uint notificationId);

protected:
    virtual void init();
    virtual void setMode(Mode mode);

    QString title() const;
    QString iconName() const;
    QString attentionIconName() const;
    QString toolTipIconName() const;
    QString iconThemePath() const;
    QString menuObjectPath() const;

    virtual bool eventFilter(QObject *watched, QEvent *event);

private slots:
    void activated(const QPoint &pos);
    void serviceChange(const QString &name, const QString &oldOwner, const QString &newOwner);
    void checkForRegisteredHosts();

    void notificationClosed(uint id, uint reason);
    void notificationInvoked(uint id, const QString &action);

private:
    void registerToDaemon();

    static const int _protocolVersion;
    static const QString _statusNotifierWatcherServiceName;
    StatusNotifierItemDBus *_statusNotifierItemDBus;

    org::kde::StatusNotifierWatcher *_statusNotifierWatcher;
    org::freedesktop::Notifications *_notificationsClient;
    bool _notificationsClientSupportsMarkup;
    bool _notificationsClientSupportsActions;
    quint32 _lastNotificationsDBusId;
    QHash<uint, uint> _notificationsIdMap; ///< Maps our own notification ID to the D-Bus one

    QString _iconThemePath;
    QString _menuObjectPath;
    bool _trayIconInverted;

    friend class StatusNotifierItemDBus;
};


#endif /* HAVE_DBUS */
#endif /* STATUSNOTIFIERITEM_H_ */
