/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
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

#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QStringList>

#include "abstractnotificationbackend.h"
#include "buffermodel.h"
#include "chatlinemodel.h"
#include "contextmenuactionprovider.h"
#include "icon.h"
#include "mainwin.h"
#include "qtuimessageprocessor.h"
#include "qtuisettings.h"
#include "qtuistyle.h"
#include "systemtray.h"
#include "toolbaractionprovider.h"
#include "types.h"
#include "util.h"

QList<AbstractNotificationBackend *> QtUi::_notificationBackends;
QList<AbstractNotificationBackend::Notification> QtUi::_notifications;


QtUi *QtUi::instance()
{
    return static_cast<QtUi*>(GraphicalUi::instance());
}


QtUi::QtUi()
    : GraphicalUi()
    , _systemIconTheme{QIcon::themeName()}
{
    QtUiSettings uiSettings;
    Quassel::loadTranslation(uiSettings.value("Locale", QLocale::system()).value<QLocale>());

    if (Quassel::isOptionSet("icontheme")) {
        _systemIconTheme = Quassel::optionValue("icontheme");
        QIcon::setThemeName(_systemIconTheme);
    }
    setupIconTheme();
    QApplication::setWindowIcon(icon::get("quassel"));

    setUiStyle(new QtUiStyle(this));
}


QtUi::~QtUi()
{
    unregisterAllNotificationBackends();
}


void QtUi::init()
{
    setContextMenuActionProvider(new ContextMenuActionProvider(this));
    setToolBarActionProvider(new ToolBarActionProvider(this));

    _mainWin.reset(new MainWin());  // TODO C++14: std::make_unique
    setMainWidget(_mainWin.get());

    connect(_mainWin.get(), SIGNAL(connectToCore(const QVariantMap &)), this, SIGNAL(connectToCore(const QVariantMap &)));
    connect(_mainWin.get(), SIGNAL(disconnectFromCore()), this, SIGNAL(disconnectFromCore()));
    connect(Client::instance(), SIGNAL(bufferMarkedAsRead(BufferId)), SLOT(closeNotifications(BufferId)));

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


std::vector<std::pair<QString, QString>> QtUi::availableIconThemes() const
{
    //: Supported icon theme names
    static const std::vector<std::pair<QString, QString>> supported {
        { "breeze", tr("Breeze") },
        { "breeze-dark", tr("Breeze Dark") },
#ifdef WITH_OXYGEN_ICONS
        { "oxygen", tr("Oxygen") }
#endif
    };

    std::vector<std::pair<QString, QString>> result;
    for (auto &&themePair : supported) {
        for (auto &&dir : QIcon::themeSearchPaths()) {
            if (QFileInfo{dir + "/" + themePair.first + "/index.theme"}.exists()) {
                result.push_back(themePair);
                break;
            }
        }
    }

    return result;
}


QString QtUi::systemIconTheme() const
{
    return _systemIconTheme;
}


void QtUi::setupIconTheme()
{
    // Add paths to our own icon sets to the theme search paths
    QStringList themePaths = QIcon::themeSearchPaths();
    themePaths.removeAll(":/icons");  // this should come last
    for (auto &&dataDir : Quassel::dataDirPaths()) {
        QString iconDir{dataDir + "icons"};
        if (QFileInfo{iconDir}.isDir()) {
            themePaths << iconDir;
        }
    }
    themePaths << ":/icons";
    QIcon::setThemeSearchPaths(themePaths);

    refreshIconTheme();
}


void QtUi::refreshIconTheme()
{
    // List of available fallback themes
    QStringList availableThemes;
    for (auto &&themePair : availableIconThemes()) {
        availableThemes << themePair.first;
    }

    if (availableThemes.isEmpty()) {
        // We could probably introduce a more sophisticated fallback handling, such as putting the "most important" icons into hicolor,
        // but this just gets complex for no good reason. We really rely on a supported theme to be installed, if not system-wide, then
        // as part of the Quassel installation (which is enabled by default anyway).
        qWarning() << tr("No supported icon theme installed, you'll lack icons! Supported are the KDE/Plasma themes Breeze, Breeze Dark and Oxygen.");
        return;
    }

    UiStyleSettings s;
    QString fallbackTheme{s.value("Icons/FallbackTheme").toString()};

    if (fallbackTheme.isEmpty() || !availableThemes.contains(fallbackTheme)) {
        if (availableThemes.contains(_systemIconTheme)) {
            fallbackTheme = _systemIconTheme;
        }
        else {
            fallbackTheme = availableThemes.first();
        }
    }

    if (_systemIconTheme.isEmpty() || _systemIconTheme == fallbackTheme || s.value("Icons/OverrideSystemTheme", true).toBool()) {
        // We have a valid fallback theme and want to override the system theme (if it's even defined), so we're basically done
        QIcon::setThemeName(fallbackTheme);
        emit iconThemeRefreshed();
        return;
    }

    // At this point, we have a system theme that we don't want to override, but that may not contain all
    // required icons.
    // We create a dummy theme that inherits first from the system theme, then from the supported fallback.
    // This rather ugly hack allows us to inject the fallback into the inheritance chain, so non-standard
    // icons missing in the system theme will be filled in by the fallback.
    // Since we can't get notified when the system theme changes, this means that a restart may be required
    // to apply a theme change... but you can't have everything, I guess.
    if (!_dummyThemeDir) {
        _dummyThemeDir.reset(new QTemporaryDir{});
        if (!_dummyThemeDir->isValid() || !QDir{_dummyThemeDir->path()}.mkpath("icons/quassel-icon-proxy/apps/32")) {
            qWarning() << "Could not create temporary directory for proxying the system icon theme, using fallback";
            QIcon::setThemeName(fallbackTheme);
            emit iconThemeRefreshed();
            return;
        }
        // Add this to XDG_DATA_DIRS, otherwise KIconLoader complains
        auto xdgDataDirs = qgetenv("XDG_DATA_DIRS");
        if (!xdgDataDirs.isEmpty())
            xdgDataDirs += ":";
        xdgDataDirs += _dummyThemeDir->path();
        qputenv("XDG_DATA_DIRS", xdgDataDirs);

        QIcon::setThemeSearchPaths(QIcon::themeSearchPaths() << _dummyThemeDir->path() + "/icons");
    }

    QFile indexFile{_dummyThemeDir->path() + "/icons/quassel-icon-proxy/index.theme"};
    if (!indexFile.open(QFile::WriteOnly|QFile::Truncate)) {
        qWarning() << "Could not create index file for proxying the system icon theme, using fallback";
        QIcon::setThemeName(fallbackTheme);
        emit iconThemeRefreshed();
        return;
    }

    // Write a dummy index file that is sufficient to make QIconLoader happy
    auto indexContents = QString{
            "[Icon Theme]\n"
            "Name=quassel-icon-proxy\n"
            "Inherits=%1,%2\n"
            "Directories=apps/32\n"
            "[apps/32]\nSize=32\nType=Fixed\n"
    }.arg(_systemIconTheme, fallbackTheme);
    if (indexFile.write(indexContents.toLatin1()) < 0) {
        qWarning() << "Could not write index file for proxying the system icon theme, using fallback";
        QIcon::setThemeName(fallbackTheme);
        emit iconThemeRefreshed();
        return;
    }
    indexFile.close();
    QIcon::setThemeName("quassel-icon-proxy");
}
