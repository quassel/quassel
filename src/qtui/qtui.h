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

#pragma once

#include <memory>
#include <tuple>
#include <vector>

#include <QList>
#include <QObject>
#include <QString>

#if QT_VERSION >= 0x050000
#  include <QTemporaryDir>
#endif

#include "abstractnotificationbackend.h"
#include "graphicalui.h"
#include "qtuistyle.h"

class MainWin;
class MessageModel;
class QtUiMessageProcessor;

//! This class encapsulates Quassel's Qt-based GUI.
/** This is basically a wrapper around MainWin, which is necessary because we cannot derive MainWin
 *  from both QMainWindow and AbstractUi (because of multiple inheritance of QObject).
 */
class QtUi : public GraphicalUi
{
    Q_OBJECT

public:
    QtUi();
    ~QtUi() override;

    MessageModel *createMessageModel(QObject *parent) override;
    AbstractMessageProcessor *createMessageProcessor(QObject *parent) override;

    static QtUi *instance();
    inline static QtUiStyle *style();
    inline static MainWin *mainWindow();

    QString debugLog() const;

    static bool haveSystemTray();

    /* Notifications */

    static void registerNotificationBackend(AbstractNotificationBackend *);
    static void unregisterNotificationBackend(AbstractNotificationBackend *);
    static void unregisterAllNotificationBackends();
    static const QList<AbstractNotificationBackend *> &notificationBackends();
    static const QList<AbstractNotificationBackend::Notification> &activeNotifications();

    /**
     * Determine available fallback icon themes.
     *
     * @returns The list of supported fallback themes (Breeze (Dark), Oxygen) that are available on the system
     */
    std::vector<std::pair<QString, QString>> availableIconThemes() const;

    /**
     * Determine the system icon theme set when Quassel was started.
     *
     * This property stores the icon theme initially configured in Qt when starting up (may be empty on platforms
     * not supporting system icon themes). If the --icontheme option is given, uses that.
     *
     * Since Qt does not support notifications on theme changes, this property will not be updated when the theme
     * changes at runtime.
     *
     * @returns The system icon theme at startup time
     */
    QString systemIconTheme() const;

public slots:
    void init() override;

    uint invokeNotification(BufferId bufId, AbstractNotificationBackend::NotificationType type, const QString &sender, const QString &text);
    void closeNotification(uint notificationId);
    void closeNotifications(BufferId bufferId = BufferId());

    /**
     * Refresh the current icon theme.
     *
     * @note This will not detect changes in the system icon theme, so if that changes, a client restart
     *       is required for icons to work correctly.
     */
    void refreshIconTheme();

signals:
    void iconThemeRefreshed();

protected slots:
    void connectedToCore() override;
    void disconnectedFromCore() override;
    void notificationActivated(uint notificationId);
    void bufferMarkedAsRead(BufferId);

protected:
    void minimizeRestore(bool show) override;
    bool isHidingMainWidgetAllowed() const override;

private slots:
    void useSystemTrayChanged(const QVariant &);

private:
    /**
     * Sets up icon theme handling.
     */
    void setupIconTheme();

private:
    static QList<AbstractNotificationBackend *> _notificationBackends;
    static QList<AbstractNotificationBackend::Notification> _notifications;

    std::unique_ptr<MainWin> _mainWin;

    QString _systemIconTheme;

#if QT_VERSION >= 0x050000
    std::unique_ptr<QTemporaryDir> _dummyThemeDir;
#endif

    bool _useSystemTray;
};

QtUiStyle *QtUi::style() { return qobject_cast<QtUiStyle *>(uiStyle()); }
MainWin *QtUi::mainWindow() { return instance()->_mainWin.get(); }
