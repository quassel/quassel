/***************************************************************************
*   Copyright (C) 2005-09 by the Quassel Project                          *
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
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

#ifndef SNORENOTIFICATIONBACKEND_H_
#define SNORENOTIFICATIONBACKEND_H_

#include "abstractnotificationbackend.h"

#include "settingspage.h"


namespace Snore{
class SnoreServer;
}

#include <snore/core/notification/notification.h>

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

private:
    class ConfigWidget;
    Snore::SnoreServer *m_snore;
    Snore::Notification::Action *m_action;
    QHash<uint,Snore::Notification> m_notifications;
    QHash<uint,uint> m_notificationIds;
};

class SnoreNotificationBackend::ConfigWidget : public SettingsPage {
    Q_OBJECT

public:
    ConfigWidget(Snore::SnoreServer *snore,QWidget *parent = 0);
    void save();
    void load();
    bool hasDefaults() const;
    void defaults();

private slots:
    void backendChanged(QString);

private:
    Snore::SnoreServer *m_snore;
    QComboBox *m_backends;
    QString m_backend;
    //  QSpinBox *timeoutBox;

    //  bool enabled;
    //  int timeout;
};

#endif
