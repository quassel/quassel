/***************************************************************************
 *   Copyright (C) 2005-2022 by the Quassel Project                        *
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

#include "systemtray.h"

#include <QApplication>
#include <QMenu>

#include "action.h"
#include "actioncollection.h"
#include "client.h"
#include "icon.h"
#include "qtui.h"

SystemTray::SystemTray(QWidget* parent)
    : QObject(parent)
    , _associatedWidget(parent)
{
    Q_ASSERT(parent);

    NotificationSettings{}.initAndNotify("Systray/ChangeColor", this, &SystemTray::enableChangeColorChanged, true);
    NotificationSettings{}.initAndNotify("Systray/Animate", this, &SystemTray::enableBlinkChanged, false);
    UiStyleSettings{}.initAndNotify("Icons/InvertTray", this, &SystemTray::invertTrayIconChanged, false);

    ActionCollection* coll = QtUi::actionCollection("General");
    _minimizeRestoreAction = new Action(tr("&Minimize"), this, this, &SystemTray::minimizeRestore);

    _trayMenu = new QMenu(associatedWidget());
    _trayMenu->setTitle("Quassel IRC");
    _trayMenu->setAttribute(Qt::WA_Hover);

    _trayMenu->addAction(coll->action("ConnectCore"));
    _trayMenu->addAction(coll->action("DisconnectCore"));
    _trayMenu->addAction(coll->action("CoreInfo"));
    _trayMenu->addSeparator();
    _trayMenu->addAction(_minimizeRestoreAction);
    _trayMenu->addAction(coll->action("Quit"));
    connect(_trayMenu, &QMenu::aboutToShow, this, &SystemTray::trayMenuAboutToShow);

    connect(QtUi::instance(), &QtUi::iconThemeRefreshed, this, &SystemTray::iconsChanged);

    _blinkTimer.setInterval(1000);
    _blinkTimer.setSingleShot(false);
    connect(&_blinkTimer, &QTimer::timeout, this, &SystemTray::onBlinkTimeout);
}

SystemTray::~SystemTray()
{
    _trayMenu->deleteLater();
}

QWidget* SystemTray::associatedWidget() const
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

        if (state == NeedsAttention && _attentionBehavior == AttentionBehavior::Blink) {
            _blinkTimer.start();
            _blinkState = true;
        }
        else {
            _blinkTimer.stop();
            _blinkState = false;
        }
        emit currentIconNameChanged();
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

QString SystemTray::currentIconName() const
{
    if (state() == State::NeedsAttention) {
        if (_attentionBehavior == AttentionBehavior::ChangeColor) {
            return iconName(State::NeedsAttention);
        }
        if (_attentionBehavior == AttentionBehavior::Blink && _blinkState) {
            return iconName(State::NeedsAttention);
        }
        return iconName(State::Active);
    }
    else {
        return iconName(state());
    }
}

QString SystemTray::currentAttentionIconName() const
{
    if (state() == State::NeedsAttention && _attentionBehavior == AttentionBehavior::Blink && !_blinkState) {
        return iconName(State::Active);
    }
    return iconName(State::NeedsAttention);
}

bool SystemTray::isAlerted() const
{
    return state() == State::NeedsAttention;
}

void SystemTray::setAlert(bool alerted)
{
    if (alerted) {
        setState(NeedsAttention);
    }
    else {
        setState(Client::isConnected() ? Active : Passive);
    }
}

void SystemTray::onBlinkTimeout()
{
    _blinkState = !_blinkState;
    emit currentIconNameChanged();
}

QMenu* SystemTray::trayMenu() const
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

void SystemTray::enableChangeColorChanged(const QVariant& v)
{
    if (v.toBool()) {
        _attentionBehavior = AttentionBehavior::ChangeColor;
    }
    else {
        if (_attentionBehavior == AttentionBehavior::ChangeColor) {
            _attentionBehavior = AttentionBehavior::DoNothing;
        }
    }
    emit currentIconNameChanged();
}

void SystemTray::enableBlinkChanged(const QVariant& v)
{
    if (v.toBool()) {
        _attentionBehavior = AttentionBehavior::Blink;
    }
    else {
        if (_attentionBehavior == AttentionBehavior::Blink) {
            _attentionBehavior = AttentionBehavior::DoNothing;
        }
    }
    emit currentIconNameChanged();
}

void SystemTray::invertTrayIconChanged(const QVariant& v)
{
    _trayIconInverted = v.toBool();
    emit iconsChanged();
}

QString SystemTray::toolTipTitle() const
{
    return _toolTipTitle;
}

QString SystemTray::toolTipSubTitle() const
{
    return _toolTipSubTitle;
}

void SystemTray::setToolTip(const QString& title, const QString& subtitle)
{
    _toolTipTitle = title;
    _toolTipSubTitle = subtitle;
    emit toolTipChanged(title, subtitle);
}

void SystemTray::showMessage(const QString& title, const QString& message, MessageIcon icon, int millisecondsTimeoutHint, uint id)
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
