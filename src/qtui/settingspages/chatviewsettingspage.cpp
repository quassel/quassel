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

#include "chatviewsettingspage.h"
#include "client.h"
#include "colorbutton.h"
#include "qtui.h"
#include "qtuistyle.h"

ChatViewSettingsPage::ChatViewSettingsPage(QWidget *parent)
    : SettingsPage(tr("Interface"), tr("Chat View"), parent)
{
    ui.setupUi(this);

#ifndef HAVE_WEBKIT
    ui.showWebPreview->hide();
    ui.showWebPreview->setEnabled(false);
#endif

    // FIXME remove with protocol v11
    if (!(Client::coreFeatures() & Quassel::SynchronizedMarkerLine)) {
        ui.autoMarkerLine->setEnabled(false);
        ui.autoMarkerLine->setChecked(true);
        ui.autoMarkerLine->setToolTip(tr("You need at 0.6 quasselcore to use this feature"));
    }

    initAutoWidgets();
}


void ChatViewSettingsPage::save()
{
    SettingsPage::save();
    QtUi::style()->generateSettingsQss();
    QtUi::style()->reload();
}
