/***************************************************************************
 *   Copyright (C) 2005-2016 by the Quassel Project                        *
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

#include "abstractnotificationbackend.h"
#include "settingspage.h"

#include "ui_systrayanimationconfigwidget.h"

class QCheckBox;

class SystrayAnimationNotificationBackend : public AbstractNotificationBackend
{
    Q_OBJECT

public:
    SystrayAnimationNotificationBackend(QObject *parent = nullptr);

    void notify(const Notification &) override;
    void close(uint notificationId) override;
    virtual SettingsPage *createConfigWidget() const override;

private slots:
    void alertChanged(const QVariant &);

private:
    bool _alert{false};
    class ConfigWidget;
};


class SystrayAnimationNotificationBackend::ConfigWidget : public SettingsPage
{
    Q_OBJECT

public:
    ConfigWidget(QWidget *parent = nullptr);
    QString settingsKey() const override;

private:
    QVariant loadAutoWidgetValue(const QString &widgetName) override;
    void saveAutoWidgetValue(const QString &widgetName, const QVariant &value) override;

private:
    Ui::SystrayAnimationConfigWidget ui;
};
