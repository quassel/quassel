/***************************************************************************
 *   Copyright (C) 2005-2022 by the Quassel Project                        *
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

#ifndef TASKBARNOTIFICATIONBACKEND_H_
#define TASKBARNOTIFICATIONBACKEND_H_

#include "abstractnotificationbackend.h"
#include "settingspage.h"

class QCheckBox;
class QSpinBox;

class TaskbarNotificationBackend : public AbstractNotificationBackend
{
    Q_OBJECT

public:
    TaskbarNotificationBackend(QObject* parent = nullptr);

    void notify(const Notification&) override;
    void close(uint notificationId) override;
    SettingsPage* createConfigWidget() const override;

private slots:
    void enabledChanged(const QVariant&);
    void timeoutChanged(const QVariant&);

private:
    class ConfigWidget;

    bool _enabled;
    int _timeout;
};

class TaskbarNotificationBackend::ConfigWidget : public SettingsPage
{
    Q_OBJECT

public:
    ConfigWidget(QWidget* parent = nullptr);
    void save() override;
    void load() override;
    bool hasDefaults() const override;
    void defaults() override;

private slots:
    void widgetChanged();

private:
    QCheckBox* enabledBox;
    QSpinBox* timeoutBox;

    bool enabled;
    int timeout;
};

#endif
