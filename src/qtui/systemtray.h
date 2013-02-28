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

#ifndef SYSTEMTRAY_H_
#define SYSTEMTRAY_H_

#include "icon.h"

class Action;
class QMenu;

class SystemTray : public QObject
{
    Q_OBJECT
    Q_ENUMS(State Mode MessageIcon ActivationReason)

public :
        enum State {
        Passive,
        Active,
        NeedsAttention
    };

    enum Mode {
        Invalid,
        Legacy,
        StatusNotifier
    };

    // same as in QSystemTrayIcon
    enum MessageIcon {
        NoIcon,
        Information,
        Warning,
        Critical
    };

    // same as in QSystemTrayIcon
    enum ActivationReason {
        Unknown,
        Context,
        DoubleClick,
        Trigger,
        MiddleClick
    };

    explicit SystemTray(QWidget *parent);
    virtual ~SystemTray();
    virtual void init();

    inline Mode mode() const;
    inline State state() const;
    inline bool isAlerted() const;
    virtual inline bool isSystemTrayAvailable() const;

    void setAlert(bool alerted);
    virtual inline bool isVisible() const { return false; }

    QWidget *associatedWidget() const;

public slots:
    virtual void setState(State);
    virtual void setVisible(bool visible = true);
    virtual void setToolTip(const QString &title, const QString &subtitle);
    virtual void showMessage(const QString &title, const QString &message, MessageIcon icon = Information, int msTimeout = 10000, uint notificationId = 0);
    virtual void closeMessage(uint notificationId) { Q_UNUSED(notificationId) }

signals:
    void activated(SystemTray::ActivationReason);
    void iconChanged(const Icon &);
    void animationEnabledChanged(bool);
    void toolTipChanged(const QString &title, const QString &subtitle);
    void messageClicked(uint notificationId);
    void messageClosed(uint notificationId);

protected slots:
    virtual void activate(SystemTray::ActivationReason = Trigger);

protected:
    virtual void setMode(Mode mode);
    inline bool shouldBeVisible() const;

    virtual Icon stateIcon() const;
    Icon stateIcon(State state) const;
    inline QString toolTipTitle() const;
    inline QString toolTipSubTitle() const;
    inline QMenu *trayMenu() const;

    inline bool animationEnabled() const;

private slots:
    void minimizeRestore();
    void trayMenuAboutToShow();
    void enableAnimationChanged(const QVariant &);

private:
    Mode _mode;
    State _state;
    bool _shouldBeVisible;

    QString _toolTipTitle, _toolTipSubTitle;
    Icon _passiveIcon, _activeIcon, _needsAttentionIcon;
    bool _animationEnabled;

    QMenu *_trayMenu;
    QWidget *_associatedWidget;
    Action *_minimizeRestoreAction;
};


// inlines

bool SystemTray::isSystemTrayAvailable() const { return false; }
bool SystemTray::isAlerted() const { return state() == NeedsAttention; }
SystemTray::Mode SystemTray::mode() const { return _mode; }
SystemTray::State SystemTray::state() const { return _state; }
bool SystemTray::shouldBeVisible() const { return _shouldBeVisible; }
QMenu *SystemTray::trayMenu() const { return _trayMenu; }
QString SystemTray::toolTipTitle() const { return _toolTipTitle; }
QString SystemTray::toolTipSubTitle() const { return _toolTipSubTitle; }
bool SystemTray::animationEnabled() const { return _animationEnabled; }

#endif
