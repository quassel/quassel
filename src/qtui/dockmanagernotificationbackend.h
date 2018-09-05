/***************************************************************************
 *   Copyright (C) 2013-2018 by the Quassel Project                        *
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

#ifndef DOCKMANAGERNOTIFICATIONBACKEND_H_
#define DOCKMANAGERNOTIFICATIONBACKEND_H_

#include <QDBusConnection>
#include <QDBusInterface>

#include "abstractnotificationbackend.h"
#include "settingspage.h"

class QCheckBox;

class DockManagerNotificationBackend : public AbstractNotificationBackend
{
    Q_OBJECT

public:
    DockManagerNotificationBackend(QObject *parent = nullptr);

    void notify(const Notification &) override;
    void close(uint notificationId) override;
    SettingsPage *createConfigWidget() const override;

private slots:
    void enabledChanged(const QVariant &);
    void updateProgress(int progress);
    void updateProgress(int done, int total);
    void itemAdded(QDBusObjectPath);
    void synchronized();

private:
    class ConfigWidget;
    bool _enabled;
    bool _available;
    QDBusConnection _bus;
    QDBusInterface *_dock;
    QDBusInterface *_item;
    int _count;
};


class DockManagerNotificationBackend::ConfigWidget : public SettingsPage
{
    Q_OBJECT

public:
    ConfigWidget(bool enabled, QWidget *parent = nullptr);

    void save() override;
    void load() override;
    bool hasDefaults() const override;
    void defaults() override;

private slots:
    void widgetChanged();

private:
    QCheckBox *enabledBox;
    bool enabled;
};


#endif
