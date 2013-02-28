/***************************************************************************
 *   Copyright (C) 2005-2013 by the Quassel Project                        *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This file is free software; you can redistribute it and/or modify     *
 *   it under the terms of the GNU Library General Public License (LGPL)   *
 *   as published by the Free Software Foundation; either version 2 of the *
 *   License, or (at your option) any later version.                       *
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

#ifndef LEGACYSYSTEMTRAY_H_
#define LEGACYSYSTEMTRAY_H_

#ifndef QT_NO_SYSTEMTRAYICON

#ifdef HAVE_KDE
#  include <KSystemTrayIcon>
#else
#  include <QSystemTrayIcon>
#endif

#include <QTimer>

#include "systemtray.h"

class LegacySystemTray : public SystemTray
{
    Q_OBJECT

public:
    explicit LegacySystemTray(QWidget *parent);
    virtual ~LegacySystemTray() {}
    virtual void init();

    virtual bool isVisible() const;
    virtual inline bool isSystemTrayAvailable() const;
    virtual Icon stateIcon() const; // overriden to care about blinkState

public slots:
    virtual void setState(State state);
    virtual void setVisible(bool visible = true);
    virtual void showMessage(const QString &title, const QString &message, MessageIcon icon = Information, int msTimeout = 10000, uint notificationId = 0);
    virtual void closeMessage(uint notificationId);

protected slots:

protected:
    virtual void setMode(Mode mode);

private slots:
    void on_blinkTimeout();
    void on_activated(QSystemTrayIcon::ActivationReason);
    void on_messageClicked();

    void syncLegacyIcon();

private:
    QTimer _blinkTimer;
    bool _blinkState;
    uint _lastMessageId;

#ifdef HAVE_KDE
    KSystemTrayIcon *_trayIcon;
#else
    QSystemTrayIcon *_trayIcon;
#endif
};


// inlines

bool LegacySystemTray::isSystemTrayAvailable() const
{
    return mode() == Legacy ? QSystemTrayIcon::isSystemTrayAvailable()
           : SystemTray::isSystemTrayAvailable();
}


#endif /* QT_NO_SYSTEMTRAYICON */

#endif /* LEGACYSYSTEMTRAY_H_ */
