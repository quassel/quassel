/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
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

#pragma once

#include <QObject>
#include <QString>

class Action;
class QMenu;

class SystemTray : public QObject
{
    Q_OBJECT
    Q_ENUMS(State Mode MessageIcon ActivationReason)

public:
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
    ~SystemTray() override;

    Mode mode() const;
    State state() const;
    bool isVisible() const;
    bool isAlerted() const;

    virtual bool isSystemTrayAvailable() const;

    QWidget *associatedWidget() const;

public slots:
    void setVisible(bool visible = true);
    void setState(State);
    void setAlert(bool alerted);

    void setToolTip(const QString &title, const QString &subtitle);
    virtual void showMessage(const QString &title, const QString &message, MessageIcon icon = Information, int msTimeout = 10000, uint notificationId = 0);
    virtual void closeMessage(uint notificationId);

signals:
    void modeChanged(Mode mode);
    void stateChanged(State state);
    void visibilityChanged(bool isVisible);

    void activated(SystemTray::ActivationReason);
    void iconChanged(const QIcon &icon);
    void animationEnabledChanged(bool);
    void toolTipChanged(const QString &title, const QString &subtitle);
    void messageClicked(uint notificationId);
    void messageClosed(uint notificationId);

protected slots:
    virtual void activate(SystemTray::ActivationReason = Trigger);

protected:
    void setMode(Mode mode);
    bool animationEnabled() const;

    QString toolTipTitle() const;
    QString toolTipSubTitle() const;
    QMenu *trayMenu() const;

    QString iconName(State state) const;

private slots:
    void minimizeRestore();
    void trayMenuAboutToShow();
    void enableAnimationChanged(const QVariant &);
    void invertTrayIconChanged(const QVariant &);

private:
    bool _isVisible{false};
    Mode _mode{Mode::Invalid};
    State _state{State::Passive};
    bool _animationEnabled{true};
    bool _trayIconInverted{false};

    QString _toolTipTitle, _toolTipSubTitle;

    QMenu *_trayMenu{nullptr};
    QWidget *_associatedWidget{nullptr};
    Action *_minimizeRestoreAction{nullptr};
};
