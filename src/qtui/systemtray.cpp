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

#include <QApplication>
#include <QMenu>

#include "systemtray.h"

#include "action.h"
#include "actioncollection.h"
#include "client.h"
#include "qtui.h"

#ifdef HAVE_KDE4
#  include <KMenu>
#  include <KWindowInfo>
#  include <KWindowSystem>
#endif

SystemTray::SystemTray(QWidget *parent)
    : QObject(parent),
    _associatedWidget(parent)
{
    Q_ASSERT(parent);

    NotificationSettings{}.initAndNotify("Systray/Animate", this, SLOT(enableAnimationChanged(QVariant)), true);
    UiStyleSettings{}.initAndNotify("Icons/InvertTray", this, SLOT(invertTrayIconChanged(QVariant)), false);

    ActionCollection *coll = QtUi::actionCollection("General");
    _minimizeRestoreAction = new Action(tr("&Minimize"), this, this, SLOT(minimizeRestore()));

#ifdef HAVE_KDE4
    KMenu *kmenu;
    _trayMenu = kmenu = new KMenu();
    kmenu->addTitle(_activeIcon, "Quassel IRC");
#else
    _trayMenu = new QMenu(associatedWidget());
#endif

    _trayMenu->setTitle("Quassel IRC");

#ifndef HAVE_KDE4
    _trayMenu->setAttribute(Qt::WA_Hover);
#endif

    _trayMenu->addAction(coll->action("ConnectCore"));
    _trayMenu->addAction(coll->action("DisconnectCore"));
    _trayMenu->addAction(coll->action("CoreInfo"));
    _trayMenu->addSeparator();
    _trayMenu->addAction(_minimizeRestoreAction);
    _trayMenu->addAction(coll->action("Quit"));

    connect(_trayMenu, SIGNAL(aboutToShow()), SLOT(trayMenuAboutToShow()));
}


SystemTray::~SystemTray()
{
    _trayMenu->deleteLater();
}


QWidget *SystemTray::associatedWidget() const
{
    return _associatedWidget;
}


bool SystemTray::isSystemTrayAvailable() const
{
    return false;
}


bool SystemTray::isVisible() const
{
    return _isVisible;
}


void SystemTray::setVisible(bool visible)
{
    if (visible != _isVisible) {
        _isVisible = visible;
        emit visibilityChanged(visible);
    }
}


SystemTray::Mode SystemTray::mode() const
{
    return _mode;
}


void SystemTray::setMode(Mode mode)
{
    if (mode != _mode) {
        _mode = mode;
#ifdef HAVE_KDE4
        if (_trayMenu) {
            if (mode == Mode::Legacy) {
                _trayMenu->setWindowFlags(Qt::Popup);
            }
            else {
                _trayMenu->setWindowFlags(Qt::Window);
            }
        }
#endif
        emit modeChanged(mode);
    }
}


SystemTray::State SystemTray::state() const
{
    return _state;
}


void SystemTray::setState(State state)
{
    if (_state != state) {
        _state = state;
        emit stateChanged(state);
    }
}


QString SystemTray::iconName(State state) const
{
    QString name;
    switch (state) {
    case State::Passive:
        name = "inactive-quassel-tray";
        break;
    case State::Active:
        name = "active-quassel-tray";
        break;
    case State::NeedsAttention:
        name = "message-quassel-tray";
        break;
    }

    if (_trayIconInverted) {
        name += "-inverted";
    }

    return name;
}


bool SystemTray::isAlerted() const
{
    return state() == State::NeedsAttention;
}


void SystemTray::setAlert(bool alerted)
{
    if (alerted)
        setState(NeedsAttention);
    else
        setState(Client::isConnected() ? Active : Passive);
}


QMenu *SystemTray::trayMenu() const
{
    return _trayMenu;
}


void SystemTray::trayMenuAboutToShow()
{
    if (GraphicalUi::isMainWidgetVisible())
        _minimizeRestoreAction->setText(tr("&Minimize"));
    else
        _minimizeRestoreAction->setText(tr("&Restore"));
}


bool SystemTray::animationEnabled() const
{
    return _animationEnabled;
}


void SystemTray::enableAnimationChanged(const QVariant &v)
{
    _animationEnabled = v.toBool();
    emit animationEnabledChanged(v.toBool());
}


void SystemTray::invertTrayIconChanged(const QVariant &v)
{
    _trayIconInverted = v.toBool();
}


QString SystemTray::toolTipTitle() const
{
    return _toolTipTitle;
}


QString SystemTray::toolTipSubTitle() const
{
    return _toolTipSubTitle;
}


void SystemTray::setToolTip(const QString &title, const QString &subtitle)
{
    _toolTipTitle = title;
    _toolTipSubTitle = subtitle;
    emit toolTipChanged(title, subtitle);
}


void SystemTray::showMessage(const QString &title, const QString &message, MessageIcon icon, int millisecondsTimeoutHint, uint id)
{
    Q_UNUSED(title)
    Q_UNUSED(message)
    Q_UNUSED(icon)
    Q_UNUSED(millisecondsTimeoutHint)
    Q_UNUSED(id)
}


void SystemTray::closeMessage(uint notificationId)
{
    Q_UNUSED(notificationId)
}


void SystemTray::activate(SystemTray::ActivationReason reason)
{
    emit activated(reason);
}


void SystemTray::minimizeRestore()
{
    GraphicalUi::toggleMainWidget();
}
