/***************************************************************************
*   Copyright (C) 2011-2013 by Patrick von Reth                           *
*   vonreth@kde.org                                                       *
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
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

#ifndef SNORENOTIFICATIONBACKEND_H_
#define SNORENOTIFICATIONBACKEND_H_

#include "abstractnotificationbackend.h"

#include "settingspage.h"

#include "ui_snorentificationconfigwidget.h"

#include <snore/core/snore.h>
#include <snore/core/notification/notification.h>

class SystrayNotificationBackend;

class SnoreNotificationBackend : public AbstractNotificationBackend {
    Q_OBJECT
public:
    SnoreNotificationBackend (QObject *parent);
    ~SnoreNotificationBackend();

    void notify(const Notification &);
    void close(uint notificationId);

    virtual SettingsPage *createConfigWidget()const;

signals:
    void activated(uint notificationId = 0);

public slots:
    void actionInvoked(Snore::Notification);
private slots:
    void backendChanged(const QVariant &);
    void timeoutChanged(const QVariant &);

private:
    void setTraybackend();
    bool setSnoreBackend(const QString &backend);

    class ConfigWidget;
    SystrayNotificationBackend * m_systrayBackend;
    Snore::SnoreCore *m_snore;
    QHash<uint, uint> m_notificationIds;
    Snore::Icon m_icon;
    Snore::Application m_application;
    Snore::Alert m_alert;
    int m_timeout;
};

class SnoreNotificationBackend::ConfigWidget : public SettingsPage {
    Q_OBJECT

public:
    ConfigWidget(Snore::SnoreCore *snore, QWidget *parent = 0);
    void save();
    void load();
    bool hasDefaults() const;
    void defaults();

private slots:
    void backendChanged(const QString&);
    void timeoutChanged(int);

private:
    Ui::SnoreNotificationConfigWidget ui;
    Snore::SnoreCore *m_snore;

    //  QSpinBox *timeoutBox;

    //  bool enabled;
    //  int timeout;
};

#endif
