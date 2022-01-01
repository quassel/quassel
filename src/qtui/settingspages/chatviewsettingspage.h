/***************************************************************************
 *   Copyright (C) 2005-2022 by the Quassel Project                        *
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

#ifndef CHATVIEWSETTINGSPAGE_H_
#define CHATVIEWSETTINGSPAGE_H_

#include "settingspage.h"

#include "ui_chatviewsettingspage.h"

class ColorButton;

class ChatViewSettingsPage : public SettingsPage
{
    Q_OBJECT

public:
    ChatViewSettingsPage(QWidget* parent = nullptr);

    inline bool hasDefaults() const override { return true; }

public slots:
    void save() override;

private:
    Ui::ChatViewSettingsPage ui;

    /**
     * Initialize the available options for sender prefixes
     */
    void initSenderPrefixComboBox();

    inline QString settingsKey() const override { return QString("QtUi/ChatView/__default__"); }
};

#endif
