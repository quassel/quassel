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

#ifndef NOTIFICATIONSSETTINGSPAGE_H_
#define NOTIFICATIONSSETTINGSPAGE_H_

#include <QHash>

#include "settingspage.h"

//! A settings page for configuring notifications
/** This class just vertically stacks the ConfigWidgets of the registered notification backends.
 *  \NOTE: When this is called, all backends need to be already registered. No dynamic changes
 *         are tracked or reacted to!
 */
class NotificationsSettingsPage : public SettingsPage
{
    Q_OBJECT

public:
    NotificationsSettingsPage(QWidget *parent = 0);

    bool hasDefaults() const;

public slots:
    void save();
    void load();
    void defaults();

private slots:
    void widgetHasChanged();

private:
    QList<SettingsPage *> _configWidgets;
    bool _hasDefaults;
};


#endif
