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

#include <QSignalMapper>

#include "itemviewsettingspage.h"
#include "colorbutton.h"
#include "qtui.h"
#include "qtuistyle.h"

ItemViewSettingsPage::ItemViewSettingsPage(QWidget *parent)
    : SettingsPage(tr("Interface"), tr("Chat & Nick Lists"), parent),
    _mapper(new QSignalMapper(this))
{
    ui.setupUi(this);

    _networkItem = new QTreeWidgetItem(ui.bufferViewPreview, QStringList(tr("Network")));
    _networkItem->setFlags(Qt::NoItemFlags);

    _inactiveBufferItem = new QTreeWidgetItem(_networkItem, QStringList(tr("Inactive")));
    _defaultBufferItem = new QTreeWidgetItem(_networkItem, QStringList(tr("Normal")));
    _unreadBufferItem = new QTreeWidgetItem(_networkItem, QStringList(tr("Unread messages")));
    _highlightedBufferItem = new QTreeWidgetItem(_networkItem, QStringList(tr("Highlight")));
    _activeBufferItem = new QTreeWidgetItem(_networkItem, QStringList(tr("Other activity")));

    ui.bufferViewPreview->expandAll();

    foreach(ColorButton *button, findChildren<ColorButton *>()) {
        connect(button, SIGNAL(colorChanged(QColor)), _mapper, SLOT(map()));
        _mapper->setMapping(button, button);
    }
    connect(_mapper, SIGNAL(mapped(QWidget *)), SLOT(updateBufferViewPreview(QWidget *)));

    initAutoWidgets();
}


void ItemViewSettingsPage::save()
{
    SettingsPage::save();
    QtUi::style()->generateSettingsQss();
    QtUi::style()->reload();
}


void ItemViewSettingsPage::updateBufferViewPreview(QWidget *widget)
{
    ColorButton *button = qobject_cast<ColorButton *>(widget);
    if (!button)
        return;

    QString objName = button->objectName();
    if (objName == "defaultBufferColor") {
        _networkItem->setForeground(0, button->color());
        _defaultBufferItem->setForeground(0, button->color());
    }
    else if (objName == "inactiveBufferColor")
        _inactiveBufferItem->setForeground(0, button->color());
    else if (objName == "activeBufferColor")
        _activeBufferItem->setForeground(0, button->color());
    else if (objName == "unreadBufferColor")
        _unreadBufferItem->setForeground(0, button->color());
    else if (objName == "highlightedBufferColor")
        _highlightedBufferItem->setForeground(0, button->color());
}
