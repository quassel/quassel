/***************************************************************************
 *   Copyright (C) 2009 Canonical Ltd                                      *
 *   author: aurelien.gateau@canonical.com                                 *
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

#ifndef INDICATORNOTIFICATIONBACKEND_H_
#define INDICATORNOTIFICATIONBACKEND_H_

#include <QHash>

#include "abstractnotificationbackend.h"
#include "settingspage.h"

#include "ui_indicatornotificationconfigwidget.h"

namespace QIndicate {
class Server;
class Indicator;
}

class Indicator;

typedef QHash<BufferId, Indicator *> IndicatorHash;

class IndicatorNotificationBackend : public AbstractNotificationBackend
{
    Q_OBJECT

public:
    IndicatorNotificationBackend(QObject *parent = 0);
    ~IndicatorNotificationBackend();

    void notify(const Notification &);
    void close(uint notificationId);
    virtual SettingsPage *createConfigWidget() const;

private slots:
    void activateMainWidget();
    void enabledChanged(const QVariant &);
    void indicatorDisplayed(QIndicate::Indicator *);

private:
    class ConfigWidget;

    bool _enabled;

    QIndicate::Server *_server;
    IndicatorHash _indicatorHash;
};


class IndicatorNotificationBackend::ConfigWidget : public SettingsPage
{
    Q_OBJECT

public:
    ConfigWidget(QWidget *parent = 0);
    ~ConfigWidget();

    void save();
    void load();
    bool hasDefaults() const;
    void defaults();

private slots:
    void widgetChanged();

private:
    Ui::IndicatorNotificationConfigWidget ui;

    bool enabled;
};


#endif
