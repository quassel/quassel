/***************************************************************************
 *   Copyright (C) 2005-2013 by the Quassel Project                        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "qtui.h"

#include "abstractnotificationbackend.h"
#include "buffermodel.h"
#include "chatlinemodel.h"
#include "contextmenuactionprovider.h"
#include "mainwin.h"
#include "qtuimessageprocessor.h"
#include "qtuisettings.h"
#include "qtuistyle.h"
#include "systemtray.h"
#include "toolbaractionprovider.h"
#include "types.h"
#include "util.h"

#ifdef Q_WS_X11
#  include <QX11Info>
#endif

QtUi *QtUi::_instance = 0;
MainWin *QtUi::_mainWin = 0;
QList<AbstractNotificationBackend *> QtUi::_notificationBackends;
QList<AbstractNotificationBackend::Notification> QtUi::_notifications;

QtUi::QtUi() : GraphicalUi()
{
    if (_instance != 0) {
        qWarning() << "QtUi has been instantiated again!";
        return;
    }
    _instance = this;

    QtUiSettings uiSettings;
    Quassel::loadTranslation(uiSettings.value("Locale", QLocale::system()).value<QLocale>());

    setContextMenuActionProvider(new ContextMenuActionProvider(this));
    setToolBarActionProvider(new ToolBarActionProvider(this));

    setUiStyle(new QtUiStyle(this));
    _mainWin = new MainWin();

    setMainWidget(_mainWin);

    connect(_mainWin, SIGNAL(connectToCore(const QVariantMap &)), this, SIGNAL(connectToCore(const QVariantMap &)));
    connect(_mainWin, SIGNAL(disconnectFromCore()), this, SIGNAL(disconnectFromCore()));
    connect(Client::instance(), SIGNAL(bufferMarkedAsRead(BufferId)), SLOT(closeNotifications(BufferId)));
}


QtUi::~QtUi()
{
    unregisterAllNotificationBackends();
    delete _mainWin;
    _mainWin = 0;
    _instance = 0;
}


void QtUi::init()
{
    _mainWin->init();
    QtUiSettings uiSettings;
    uiSettings.initAndNotify("UseSystemTrayIcon", this, SLOT(useSystemTrayChanged(QVariant)), true);

    GraphicalUi::init(); // needs to be called after the mainWin is initialized
}


MessageModel *QtUi::createMessageModel(QObject *parent)
{
    return new ChatLineModel(parent);
}


AbstractMessageProcessor *QtUi::createMessageProcessor(QObject *parent)
{
    return new QtUiMessageProcessor(parent);
}


void QtUi::connectedToCore()
{
    _mainWin->connectedToCore();
}


void QtUi::disconnectedFromCore()
{
    _mainWin->disconnectedFromCore();
    GraphicalUi::disconnectedFromCore();
}


void QtUi::useSystemTrayChanged(const QVariant &v)
{
    _useSystemTray = v.toBool();
    SystemTray *tray = mainWindow()->systemTray();
    if (_useSystemTray) {
        if (tray->isSystemTrayAvailable())
            tray->setVisible(true);
    }
    else {
        if (tray->isSystemTrayAvailable() && mainWindow()->isVisible())
            tray->setVisible(false);
    }
}


bool QtUi::haveSystemTray()
{
    return mainWindow()->systemTray()->isSystemTrayAvailable() && instance()->_useSystemTray;
}


bool QtUi::isHidingMainWidgetAllowed() const
{
    return haveSystemTray();
}


void QtUi::minimizeRestore(bool show)
{
    SystemTray *tray = mainWindow()->systemTray();
    if (show) {
        if (tray && !_useSystemTray)
            tray->setVisible(false);
    }
    else {
        if (tray && _useSystemTray)
            tray->setVisible(true);
    }
    GraphicalUi::minimizeRestore(show);
}


void QtUi::registerNotificationBackend(AbstractNotificationBackend *backend)
{
    if (!_notificationBackends.contains(backend)) {
        _notificationBackends.append(backend);
        instance()->connect(backend, SIGNAL(activated(uint)), SLOT(notificationActivated(uint)));
    }
}


void QtUi::unregisterNotificationBackend(AbstractNotificationBackend *backend)
{
    _notificationBackends.removeAll(backend);
}


void QtUi::unregisterAllNotificationBackends()
{
    _notificationBackends.clear();
}


const QList<AbstractNotificationBackend *> &QtUi::notificationBackends()
{
    return _notificationBackends;
}


uint QtUi::invokeNotification(BufferId bufId, AbstractNotificationBackend::NotificationType type, const QString &sender, const QString &text)
{
    static int notificationId = 0;

    AbstractNotificationBackend::Notification notification(++notificationId, bufId, type, sender, text);
    _notifications.append(notification);
    foreach(AbstractNotificationBackend *backend, _notificationBackends)
    backend->notify(notification);
    return notificationId;
}


void QtUi::closeNotification(uint notificationId)
{
    QList<AbstractNotificationBackend::Notification>::iterator i = _notifications.begin();
    while (i != _notifications.end()) {
        if (i->notificationId == notificationId) {
            foreach(AbstractNotificationBackend *backend, _notificationBackends)
            backend->close(notificationId);
            i = _notifications.erase(i);
        }
        else ++i;
    }
}


void QtUi::closeNotifications(BufferId bufferId)
{
    QList<AbstractNotificationBackend::Notification>::iterator i = _notifications.begin();
    while (i != _notifications.end()) {
        if (!bufferId.isValid() || i->bufferId == bufferId) {
            foreach(AbstractNotificationBackend *backend, _notificationBackends)
            backend->close(i->notificationId);
            i = _notifications.erase(i);
        }
        else ++i;
    }
}


const QList<AbstractNotificationBackend::Notification> &QtUi::activeNotifications()
{
    return _notifications;
}


void QtUi::notificationActivated(uint notificationId)
{
    if (notificationId != 0) {
        QList<AbstractNotificationBackend::Notification>::iterator i = _notifications.begin();
        while (i != _notifications.end()) {
            if (i->notificationId == notificationId) {
                BufferId bufId = i->bufferId;
                if (bufId.isValid())
                    Client::bufferModel()->switchToBuffer(bufId);
                break;
            }
            ++i;
        }
    }
    closeNotification(notificationId);

    activateMainWidget();
}


void QtUi::bufferMarkedAsRead(BufferId bufferId)
{
    if (bufferId.isValid()) {
        closeNotifications(bufferId);
    }
}
