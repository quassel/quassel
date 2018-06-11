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
    _passiveIcon(QIcon::fromTheme("inactive-quassel", QIcon(":/icons/inactive-quassel.png"))),
    _activeIcon(QIcon::fromTheme("quassel", QIcon(":/icons/quassel.png"))),
    _needsAttentionIcon(QIcon::fromTheme("message-quassel", QIcon(":/icons/message-quassel.png"))),
    _associatedWidget(parent)
{
    Q_ASSERT(parent);
}


SystemTray::~SystemTray()
{
    _trayMenu->deleteLater();
}


void SystemTray::init()
{
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

    NotificationSettings notificationSettings;
    notificationSettings.initAndNotify("Systray/Animate", this, SLOT(enableAnimationChanged(QVariant)), true);
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
    return false;
}


bool SystemTray::shouldBeVisible() const
{
    return _shouldBeVisible;
}


void SystemTray::setVisible(bool visible)
{
    _shouldBeVisible = visible;
}


SystemTray::Mode SystemTray::mode() const
{
    return _mode;
}


void SystemTray::setMode(Mode mode_)
{
    if (mode_ != _mode) {
        _mode = mode_;
#ifdef HAVE_KDE4
        if (_trayMenu) {
            if (_mode == Legacy) {
                _trayMenu->setWindowFlags(Qt::Popup);
            }
            else {
                _trayMenu->setWindowFlags(Qt::Window);
            }
        }
#endif
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
    }
}


QIcon SystemTray::stateIcon() const
{
    return stateIcon(state());
}


QIcon SystemTray::stateIcon(State state) const
{
    switch (state) {
    case Passive:
        return _passiveIcon;
    case Active:
        return _activeIcon;
    case NeedsAttention:
        return _needsAttentionIcon;
    }
    return QIcon();
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
