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
#include "systemtray.h"

class QCheckBox;

class SystrayAnimationNotificationBackend : public AbstractNotificationBackend
{
    Q_OBJECT

public:
    SystrayAnimationNotificationBackend(QObject *parent = 0);

    void notify(const Notification &);
    void close(uint notificationId);
    virtual SettingsPage *createConfigWidget() const;

private slots:
    void animateChanged(const QVariant &);

private:
    class ConfigWidget;
    bool _animate;
};


class SystrayAnimationNotificationBackend::ConfigWidget : public SettingsPage
{
    Q_OBJECT

public:
    ConfigWidget(QWidget *parent = 0);
    void save();
    void load();
    bool hasDefaults() const;
    void defaults();

private slots:
    void widgetChanged();

private:
    QCheckBox *_animateBox;
    bool _animate;
};