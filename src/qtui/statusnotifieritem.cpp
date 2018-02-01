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

#ifdef HAVE_DBUS

#include <QApplication>
#include <QMenu>
#include <QMouseEvent>
#include <QTextDocument>

#include "quassel.h"
#include "statusnotifieritem.h"
#include "statusnotifieritemdbus.h"

const int StatusNotifierItem::_protocolVersion = 0;
const QString StatusNotifierItem::_statusNotifierWatcherServiceName("org.kde.StatusNotifierWatcher");

#ifdef HAVE_DBUSMENU
#  include "dbusmenuexporter.h"

/**
 * Specialization to provide access to icon names
 */
class QuasselDBusMenuExporter : public DBusMenuExporter
{
public:
    QuasselDBusMenuExporter(const QString &dbusObjectPath, QMenu *menu, const QDBusConnection &dbusConnection)
        : DBusMenuExporter(dbusObjectPath, menu, dbusConnection)
    {}

protected:
    virtual QString iconNameForAction(QAction *action) // TODO Qt 4.7: fixme when we have converted our iconloader
    {
        QIcon icon(action->icon());
        return icon.isNull() ? QString() : icon.name();
    }
};


#endif /* HAVE_DBUSMENU */

StatusNotifierItem::StatusNotifierItem(QWidget *parent)
    : StatusNotifierItemParent(parent),
    _statusNotifierItemDBus(0),
    _statusNotifierWatcher(0),
    _notificationsClient(0),
    _notificationsClientSupportsMarkup(true),
    _lastNotificationsDBusId(0)
{
}


StatusNotifierItem::~StatusNotifierItem()
{
    delete _statusNotifierWatcher;
}


void StatusNotifierItem::init()
{
    qDBusRegisterMetaType<DBusImageStruct>();
    qDBusRegisterMetaType<DBusImageVector>();
    qDBusRegisterMetaType<DBusToolTipStruct>();

    _statusNotifierItemDBus = new StatusNotifierItemDBus(this);

    connect(this, SIGNAL(toolTipChanged(QString, QString)), _statusNotifierItemDBus, SIGNAL(NewToolTip()));
    connect(this, SIGNAL(animationEnabledChanged(bool)), _statusNotifierItemDBus, SIGNAL(NewAttentionIcon()));

    QDBusServiceWatcher *watcher = new QDBusServiceWatcher(_statusNotifierWatcherServiceName,
        QDBusConnection::sessionBus(),
        QDBusServiceWatcher::WatchForOwnerChange,
        this);
    connect(watcher, SIGNAL(serviceOwnerChanged(QString, QString, QString)), SLOT(serviceChange(QString, QString, QString)));

    setMode(StatusNotifier);

    _notificationsClient = new org::freedesktop::Notifications("org.freedesktop.Notifications", "/org/freedesktop/Notifications",
        QDBusConnection::sessionBus(), this);

    connect(_notificationsClient, SIGNAL(NotificationClosed(uint, uint)), SLOT(notificationClosed(uint, uint)));
    connect(_notificationsClient, SIGNAL(ActionInvoked(uint, QString)), SLOT(notificationInvoked(uint, QString)));

    if (_notificationsClient->isValid()) {
        QStringList desktopCapabilities = _notificationsClient->GetCapabilities();
        _notificationsClientSupportsMarkup = desktopCapabilities.contains("body-markup");
        _notificationsClientSupportsActions = desktopCapabilities.contains("actions");
    }

    StatusNotifierItemParent::init();
    trayMenu()->installEventFilter(this);

    // use the appdata icon folder for now
    _iconThemePath = Quassel::findDataFilePath("icons/extra");

#ifdef HAVE_DBUSMENU
    _menuObjectPath = "/MenuBar";
    new QuasselDBusMenuExporter(menuObjectPath(), trayMenu(), _statusNotifierItemDBus->dbusConnection()); // will be added as menu child
#endif
}


void StatusNotifierItem::registerToDaemon()
{
    if (!_statusNotifierWatcher) {
        _statusNotifierWatcher = new org::kde::StatusNotifierWatcher(_statusNotifierWatcherServiceName,
            "/StatusNotifierWatcher",
            QDBusConnection::sessionBus());
        connect(_statusNotifierWatcher, SIGNAL(StatusNotifierHostRegistered()), SLOT(checkForRegisteredHosts()));
        connect(_statusNotifierWatcher, SIGNAL(StatusNotifierHostUnregistered()), SLOT(checkForRegisteredHosts()));
    }
    if (_statusNotifierWatcher->isValid()
        && _statusNotifierWatcher->property("ProtocolVersion").toInt() == _protocolVersion) {
        _statusNotifierWatcher->RegisterStatusNotifierItem(_statusNotifierItemDBus->service());
        checkForRegisteredHosts();
    }
    else {
        //qDebug() << "StatusNotifierWatcher not reachable!";
        setMode(Legacy);
    }
}


void StatusNotifierItem::serviceChange(const QString &name, const QString &oldOwner, const QString &newOwner)
{
    Q_UNUSED(name);
    if (newOwner.isEmpty()) {
        //unregistered
        //qDebug() << "Connection to the StatusNotifierWatcher lost";
        delete _statusNotifierWatcher;
        _statusNotifierWatcher = 0;
        setMode(Legacy);
    }
    else if (oldOwner.isEmpty()) {
        //registered
        setMode(StatusNotifier);
    }
}


void StatusNotifierItem::checkForRegisteredHosts()
{
    if (!_statusNotifierWatcher || !_statusNotifierWatcher->property("IsStatusNotifierHostRegistered").toBool())
        setMode(Legacy);
    else
        setMode(StatusNotifier);
}


bool StatusNotifierItem::isSystemTrayAvailable() const
{
    if (mode() == StatusNotifier)
        return true;  // else it should be set to legacy on registration

    return StatusNotifierItemParent::isSystemTrayAvailable();
}


bool StatusNotifierItem::isVisible() const
{
    if (mode() == StatusNotifier)
        return shouldBeVisible();  // we don't have a way to check, so we need to trust everything went right

    return StatusNotifierItemParent::isVisible();
}


void StatusNotifierItem::setMode(Mode mode_)
{
    if (mode_ == mode())
        return;

    if (mode_ != StatusNotifier) {
        _statusNotifierItemDBus->unregisterService();
    }

    StatusNotifierItemParent::setMode(mode_);

    if (mode() == StatusNotifier) {
        _statusNotifierItemDBus->registerService();
        registerToDaemon();
    }
}


void StatusNotifierItem::setState(State state_)
{
    StatusNotifierItemParent::setState(state_);

    emit _statusNotifierItemDBus->NewStatus(metaObject()->enumerator(metaObject()->indexOfEnumerator("State")).valueToKey(state()));
    emit _statusNotifierItemDBus->NewIcon();
}


void StatusNotifierItem::setVisible(bool visible)
{
    if (visible == isVisible())
        return;

    LegacySystemTray::setVisible(visible);

    if (mode() == StatusNotifier) {
        if (shouldBeVisible()) {
            _statusNotifierItemDBus->registerService();
            registerToDaemon();
        }
        else {
            _statusNotifierItemDBus->unregisterService();
            _statusNotifierWatcher->deleteLater();
            _statusNotifierWatcher = 0;
        }
    }
}


QString StatusNotifierItem::title() const
{
    return QString("Quassel IRC");
}


QString StatusNotifierItem::iconName() const
{
    if (state() == Passive)
        return QString("inactive-quassel-tray");
    else
        return QString("active-quassel-tray");
}


QString StatusNotifierItem::attentionIconName() const
{
    if (animationEnabled())
        return QString("message-quassel-tray");
    else
        return QString("active-quassel-tray");
}


QString StatusNotifierItem::toolTipIconName() const
{
    return QString("active-quassel-tray");
}


QString StatusNotifierItem::iconThemePath() const
{
    return _iconThemePath;
}


QString StatusNotifierItem::menuObjectPath() const
{
    return _menuObjectPath;
}


void StatusNotifierItem::activated(const QPoint &pos)
{
    Q_UNUSED(pos)
    activate(Trigger);
}


bool StatusNotifierItem::eventFilter(QObject *watched, QEvent *event)
{
    if (mode() == StatusNotifier) {
        //FIXME: ugly ugly workaround to weird QMenu's focus problems
#ifdef HAVE_KDE4
        if (watched == trayMenu() &&
            (event->type() == QEvent::WindowDeactivate || (event->type() == QEvent::MouseButtonRelease && static_cast<QMouseEvent *>(event)->button() == Qt::LeftButton))) {
            // put at the back of event queue to let the action activate anyways
            QTimer::singleShot(0, trayMenu(), SLOT(hide()));
        }
#else
        if (watched == trayMenu() && event->type() == QEvent::HoverLeave) {
            trayMenu()->hide();
        }
#endif
    }
    return StatusNotifierItemParent::eventFilter(watched, event);
}


void StatusNotifierItem::showMessage(const QString &title, const QString &message_, SystemTray::MessageIcon icon, int timeout, uint notificationId)
{
    QString message = message_;
    if (_notificationsClient->isValid()) {
        if (_notificationsClientSupportsMarkup)
#if QT_VERSION < 0x050000
            message = Qt::escape(message);
#else
            message = message.toHtmlEscaped();
#endif

        QStringList actions;
        if (_notificationsClientSupportsActions)
            actions << "activate" << "View";

        // we always queue notifications right now
        QDBusReply<uint> reply = _notificationsClient->Notify(title, 0, "quassel", title, message, actions, QVariantMap(), timeout);
        if (reply.isValid()) {
            uint dbusid = reply.value();
            _notificationsIdMap.insert(dbusid, notificationId);
            _lastNotificationsDBusId = dbusid;
        }
    }
    else
        StatusNotifierItemParent::showMessage(title, message, icon, timeout, notificationId);
}


void StatusNotifierItem::closeMessage(uint notificationId)
{
    foreach(uint dbusid, _notificationsIdMap.keys()) {
        if (_notificationsIdMap.value(dbusid) == notificationId) {
            _notificationsIdMap.remove(dbusid);
            _notificationsClient->CloseNotification(dbusid);
        }
    }
    _lastNotificationsDBusId = 0;
}


void StatusNotifierItem::notificationClosed(uint dbusid, uint reason)
{
    Q_UNUSED(reason)
    _lastNotificationsDBusId = 0;
    emit messageClosed(_notificationsIdMap.take(dbusid));
}


void StatusNotifierItem::notificationInvoked(uint dbusid, const QString &action)
{
    Q_UNUSED(action)
    emit messageClicked(_notificationsIdMap.value(dbusid, 0));
}


#endif
