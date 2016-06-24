/***************************************************************************
 *   Copyright (C) 2011-2016 by Hannah von Reth                            *
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

#include <libsnore/snore.h>
#include <libsnore/notification/notification.h>

class SystrayNotificationBackend;

class SnoreNotificationBackend : public AbstractNotificationBackend {
    Q_OBJECT
public:
    SnoreNotificationBackend (QObject *parent);
    ~SnoreNotificationBackend();

    void notify(const Notification &);
    void close(uint notificationId);

    virtual SettingsPage *createConfigWidget() const;

public slots:
    void actionInvoked(Snore::Notification);

private slots:
    void setTraybackend(const QVariant &b);

private:

    class ConfigWidget;
#ifndef HAVE_KDE
    SystrayNotificationBackend * m_systrayBackend = nullptr;
#endif
    QHash<uint, uint> m_notificationIds;
    Snore::Icon m_icon;
    Snore::Application m_application;
    Snore::Alert m_alert;
};

class SnoreNotificationBackend::ConfigWidget : public SettingsPage {
    Q_OBJECT

public:
    ConfigWidget(QWidget *parent = 0);

    bool hasDefaults() const;
    void defaults();
    void load();
    void save();
private slots:
    void useSnnoreChanged(bool);

private:
    Ui::SnoreNotificationConfigWidget ui;
};

#endif
