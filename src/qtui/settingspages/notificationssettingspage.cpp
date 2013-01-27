/***************************************************************************
 *   Copyright (C) 2005-2013 by the Quassel Project                        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include <QVBoxLayout>

#include "notificationssettingspage.h"

#include "qtui.h"

NotificationsSettingsPage::NotificationsSettingsPage(QWidget *parent)
    : SettingsPage(tr("Interface"), tr("Notifications"), parent),
    _hasDefaults(false)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    foreach(AbstractNotificationBackend *backend, QtUi::notificationBackends()) {
        SettingsPage *cw = backend->createConfigWidget();
        if (cw) {
            cw->setParent(this);
            _configWidgets.append(cw);
            layout->addWidget(cw);
            connect(cw, SIGNAL(changed(bool)), SLOT(widgetHasChanged()));
            _hasDefaults |= cw->hasDefaults();
        }
    }
    layout->addStretch(20);
    load();
}


bool NotificationsSettingsPage::hasDefaults() const
{
    return _hasDefaults;
}


void NotificationsSettingsPage::defaults()
{
    foreach(SettingsPage *cw, _configWidgets)
    cw->defaults();
    widgetHasChanged();
}


void NotificationsSettingsPage::load()
{
    foreach(SettingsPage *cw, _configWidgets)
    cw->load();
    setChangedState(false);
}


void NotificationsSettingsPage::save()
{
    foreach(SettingsPage *cw, _configWidgets)
    cw->save();
    setChangedState(false);
}


void NotificationsSettingsPage::widgetHasChanged()
{
    bool changed = false;
    foreach(SettingsPage *cw, _configWidgets) {
        if (cw->hasChanged()) {
            changed = true;
            break;
        }
    }
    if (changed != hasChanged()) setChangedState(changed);
}
