/***************************************************************************
 *   Copyright (C) 2005-2019 by the Quassel Project                        *
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

#include "statusnotifieritem.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QIcon>
#include <QMenu>
#include <QMouseEvent>
#include <QTextDocument>

#include "icon.h"
#include "qtui.h"
#include "quassel.h"
#include "statusnotifieritemdbus.h"

constexpr int kProtocolVersion {0};

const QString kSniWatcherService       {QLatin1String{"org.kde.StatusNotifierWatcher"}};
const QString kSniWatcherPath          {QLatin1String{"/StatusNotifierWatcher"}};
const QString kSniPath                 {QLatin1String{"/StatusNotifierItem"}};
const QString kXdgNotificationsService {QLatin1String{"org.freedesktop.Notifications"}};
const QString kXdgNotificationsPath    {QLatin1String{"/org/freedesktop/Notifications"}};
const QString kMenuObjectPath          {QLatin1String{"/MenuBar"}};

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
    : StatusNotifierItemParent(parent)
#if QT_VERSION >= 0x050000
    , _iconThemeDir{QDir::tempPath() + QLatin1String{"/quassel-sni-XXXXXX"}}
#endif
{
    static bool registered = []() -> bool {
        qDBusRegisterMetaType<DBusImageStruct>();
        qDBusRegisterMetaType<DBusImageVector>();
        qDBusRegisterMetaType<DBusToolTipStruct>();
        return true;
    }();
    Q_UNUSED(registered)

    setMode(Mode::StatusNotifier);

    connect(this, SIGNAL(visibilityChanged(bool)), this, SLOT(onVisibilityChanged(bool)));
    connect(this, SIGNAL(modeChanged(Mode)), this, SLOT(onModeChanged(Mode)));
    connect(this, SIGNAL(stateChanged(State)), this, SLOT(onStateChanged(State)));

    trayMenu()->installEventFilter(this);

    // Create a temporary directory that holds copies of the tray icons. That way, visualizers can find our icons.
    // For Qt4 the relevant icons are installed in hicolor already, so nothing to be done.
#if QT_VERSION >= 0x050000
    if (_iconThemeDir.isValid()) {
        _iconThemePath = _iconThemeDir.path();
    }
    else {
        qWarning() << "Could not create temporary directory for themed tray icons!";
    }
#endif

    connect(this, SIGNAL(iconsChanged()), this, SLOT(refreshIcons()));
    refreshIcons();

    // Our own SNI service
    _statusNotifierItemDBus = new StatusNotifierItemDBus(this);
    connect(this, SIGNAL(currentIconNameChanged()), _statusNotifierItemDBus, SIGNAL(NewIcon()));
    connect(this, SIGNAL(currentIconNameChanged()), _statusNotifierItemDBus, SIGNAL(NewAttentionIcon()));
    connect(this, SIGNAL(toolTipChanged(QString, QString)), _statusNotifierItemDBus, SIGNAL(NewToolTip()));

    // Service watcher to keep track of the StatusNotifierWatcher service
    _serviceWatcher = new QDBusServiceWatcher(kSniWatcherService,
                                              QDBusConnection::sessionBus(),
                                              QDBusServiceWatcher::WatchForOwnerChange,
                                              this);
    connect(_serviceWatcher, SIGNAL(serviceOwnerChanged(QString, QString, QString)), SLOT(serviceChange(QString, QString, QString)));

    // Client instance for StatusNotifierWatcher
    _statusNotifierWatcher = new org::kde::StatusNotifierWatcher(kSniWatcherService,
                                                                 kSniWatcherPath,
                                                                 QDBusConnection::sessionBus(),
                                                                 this);
    connect(_statusNotifierWatcher, SIGNAL(StatusNotifierHostRegistered()), SLOT(checkForRegisteredHosts()));
    connect(_statusNotifierWatcher, SIGNAL(StatusNotifierHostUnregistered()), SLOT(checkForRegisteredHosts()));

    // Client instance for notifications
    _notificationsClient = new org::freedesktop::Notifications(kXdgNotificationsService,
                                                               kXdgNotificationsPath,
                                                               QDBusConnection::sessionBus(),
                                                               this);
    connect(_notificationsClient, SIGNAL(NotificationClosed(uint, uint)), SLOT(notificationClosed(uint, uint)));
    connect(_notificationsClient, SIGNAL(ActionInvoked(uint, QString)), SLOT(notificationInvoked(uint, QString)));

    if (_notificationsClient->isValid()) {
        QStringList desktopCapabilities = _notificationsClient->GetCapabilities();
        _notificationsClientSupportsMarkup = desktopCapabilities.contains("body-markup");
        _notificationsClientSupportsActions = desktopCapabilities.contains("actions");
    }

#ifdef HAVE_DBUSMENU
    new QuasselDBusMenuExporter(menuObjectPath(), trayMenu(), _statusNotifierItemDBus->dbusConnection()); // will be added as menu child
#endif
}


void StatusNotifierItem::serviceChange(const QString &name, const QString &oldOwner, const QString &newOwner)
{
    Q_UNUSED(name);
    if (newOwner.isEmpty()) {
        //unregistered
        setMode(Mode::Legacy);
    }
    else if (oldOwner.isEmpty()) {
        //registered
        setMode(Mode::StatusNotifier);
    }
}


void StatusNotifierItem::registerToWatcher()
{
    if (_statusNotifierWatcher->isValid() && _statusNotifierWatcher->property("ProtocolVersion").toInt() == kProtocolVersion) {
        auto registerMethod = QDBusMessage::createMethodCall(kSniWatcherService, kSniWatcherPath, kSniWatcherService,
                                                             QLatin1String{"RegisterStatusNotifierItem"});
        registerMethod.setArguments(QVariantList() << _statusNotifierItemDBus->service());
        _statusNotifierItemDBus->dbusConnection().callWithCallback(registerMethod, this, SLOT(checkForRegisteredHosts()), SLOT(onDBusError(QDBusError)));
    }
    else {
        setMode(Mode::Legacy);
    }
}


void StatusNotifierItem::checkForRegisteredHosts()
{
    if (!_statusNotifierWatcher || !_statusNotifierWatcher->property("IsStatusNotifierHostRegistered").toBool()) {
        setMode(Mode::Legacy);
    }
    else {
        setMode(Mode::StatusNotifier);
    }
}


void StatusNotifierItem::onDBusError(const QDBusError &error)
{
    qWarning() << "StatusNotifierItem encountered a D-Bus error:" << error;
    setMode(Mode::Legacy);
}


void StatusNotifierItem::refreshIcons()
{
#if QT_VERSION >= 0x050000
    if (!_iconThemePath.isEmpty()) {
        QDir baseDir{_iconThemePath + "/hicolor"};
        baseDir.removeRecursively();
        for (auto &&trayState : { State::Active, State::Passive, State::NeedsAttention }) {
            auto iconName = SystemTray::iconName(trayState);
            QIcon icon = icon::get(iconName);
            if (!icon.isNull()) {
                for (auto &&size : icon.availableSizes()) {
                    auto pixDir = QString{"%1/%2x%3/status"}.arg(baseDir.absolutePath()).arg(size.width()).arg(size.height());
                    QDir{}.mkpath(pixDir);
                    if (!icon.pixmap(size).save(pixDir + "/" + iconName + ".png")) {
                        qWarning() << "Could not save tray icon" << iconName << "for size" << size;
                    }
                }
            }
            else {
                // No theme icon found; use fallback from resources
                auto iconDir = QString{"%1/24x24/status"}.arg(baseDir.absolutePath());
                QDir{}.mkpath(iconDir);
                if (!QFile::copy(QString{":/icons/hicolor/24x24/status/%1.svg"}.arg(iconName),
                                 QString{"%1/%2.svg"}.arg(iconDir, iconName))) {
                    qWarning() << "Could not access fallback tray icon" << iconName;
                    continue;
                }
            }
        }
    }
#endif
    if (_statusNotifierItemDBus) {
        emit _statusNotifierItemDBus->NewIcon();
        emit _statusNotifierItemDBus->NewAttentionIcon();
    }
}


bool StatusNotifierItem::isSystemTrayAvailable() const
{
    if (mode() == Mode::StatusNotifier) {
        return true;  // else it should be set to legacy on registration
    }

    return StatusNotifierItemParent::isSystemTrayAvailable();
}


void StatusNotifierItem::onModeChanged(Mode mode)
{
    if (mode == Mode::StatusNotifier) {
        _statusNotifierItemDBus->registerTrayIcon();
        registerToWatcher();
    }
    else {
        _statusNotifierItemDBus->unregisterTrayIcon();
    }
}


void StatusNotifierItem::onStateChanged(State state)
{
    if (mode() == Mode::StatusNotifier) {
        emit _statusNotifierItemDBus->NewStatus(metaObject()->enumerator(metaObject()->indexOfEnumerator("State")).valueToKey(state));
    }
}


void StatusNotifierItem::onVisibilityChanged(bool isVisible)
{
    if (mode() == Mode::StatusNotifier) {
        if (isVisible) {
            _statusNotifierItemDBus->registerTrayIcon();
            registerToWatcher();
        }
        else {
            _statusNotifierItemDBus->unregisterTrayIcon();
        }
    }
}


QString StatusNotifierItem::title() const
{
    return QString("Quassel IRC");
}


QString StatusNotifierItem::iconName() const
{
    return currentIconName();
}


QString StatusNotifierItem::attentionIconName() const
{
    return currentAttentionIconName();
}


QString StatusNotifierItem::toolTipIconName() const
{
    return "quassel";
}


QString StatusNotifierItem::iconThemePath() const
{
    return _iconThemePath;
}


QString StatusNotifierItem::menuObjectPath() const
{
    return kMenuObjectPath;
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
    for (auto &&dbusid : _notificationsIdMap.keys()) {
        if (_notificationsIdMap.value(dbusid) == notificationId) {
            _notificationsIdMap.remove(dbusid);
            _notificationsClient->CloseNotification(dbusid);
        }
    }
    _lastNotificationsDBusId = 0;

    StatusNotifierItemParent::closeMessage(notificationId);
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
