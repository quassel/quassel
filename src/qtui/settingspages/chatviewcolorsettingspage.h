/***************************************************************************
 *   Copyright (C) 2005-2016 by the Quassel Project                        *
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

#pragma once

#include "settingspage.h"
#include "ui_chatviewcolorsettingspage.h"

class ColorButton;

class ChatViewColorSettingsPage : public SettingsPage
{
    Q_OBJECT

public:
    /**
     * Construct and initialize the ChatViewColorSettingsPage
     *
     * @param parent Parent QWidget object, such as the settings dialog
     */
    explicit ChatViewColorSettingsPage(QWidget *parent = 0);

    /**
     * Gets whether or not this settings page has defaults
     *
     * @return True if defaults available, otherwise false
     */
    inline bool hasDefaults() const { return true; }

public slots:
    /**
     * Save and apply current settings
     */
    void save();

private:
    Ui::ChatViewColorSettingsPage ui;  /// Reference to the Qt settings page UI

    /**
     * Gets the settings path for configuration values
     *
     * @return QString pointing to settings group and key for configuration values
     */
    inline QString settingsKey() const { return QString("QtUi/ChatView/__default__"); }
};
