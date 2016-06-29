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

#include "chatviewcolorsettingspage.h"

#include "client.h"
#include "colorbutton.h"
#include "qtui.h"
#include "qtuistyle.h"

ChatViewColorSettingsPage::ChatViewColorSettingsPage(QWidget *parent) :
    SettingsPage(tr("Interface"), tr("Chat View Colors"), parent)
{
    ui.setupUi(this);

    initAutoWidgets();
}


void ChatViewColorSettingsPage::save()
{
    // Save the general settings
    SettingsPage::save();
    // Update the stylesheet in case colors are changed
    QtUi::style()->generateSettingsQss();
    QtUi::style()->reload();
}
